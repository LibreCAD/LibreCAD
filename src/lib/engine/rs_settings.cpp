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

// RVT_PORT changed QSettings s(QSettings::Ini) to QSettings s("./qcad.ini", QSettings::IniFormat);

#include "rs_settings.h"

RS_Settings* RS_Settings::uniqueInstance = NULL;


RS_Settings::RS_Settings() {
    initialized = false;
    companyKey = "";
    appKey = "";
    group = "";
}

/**
 * Initialisation.
 *
 * @param companyKey String that identifies the company. Must start
 *        with a "/". E.g. "/RibbonSoft"
 * @param appKey String that identifies the application. Must start
 *        with a "/". E.g. "/LibreCAD"
 */
void RS_Settings::init(const QString& companyKey,
                       const QString& appKey) {

    group = "";
	
    this->companyKey = companyKey;
    this->appKey = appKey;

    //insertSearchPath(QSettings::Windows, companyKey + appKey);
    //insertSearchPath(QSettings::Unix, "/usr/share/");
    initialized = true;
}


/**
 * Destructor
 */
RS_Settings::~RS_Settings() {
    while (!cache.isEmpty()) {
        QVariant *value = *cache.begin();
        cache.erase(cache.begin());
        delete value;
    }
}



void RS_Settings::beginGroup(const QString& group) {
    this->group = group;
}

void RS_Settings::endGroup() {
    this->group = "";
}

bool RS_Settings::writeEntry(const QString& key, int value) {
    return writeEntry(key, QVariant(value));
}

bool RS_Settings::writeEntry(const QString& key,const QString& value) {
    return writeEntry(key, QVariant(value));
}

bool RS_Settings::writeEntry(const QString& key, double value) {
    return writeEntry(key, QVariant(value));
}

bool RS_Settings::writeEntry(const QString& key, const QVariant& value) {
	QSettings s(companyKey, appKey);
    // RVT_PORT not supported anymore s.insertSearchPath(QSettings::Windows, companyKey);

    s.setValue(QString("%1%2").arg(group).arg(key), value);

	addToCache(key, value);
	
	// RVT_PORT todo, remove bool 
    return true;
}

QString RS_Settings::readEntry(const QString& key,
                                 const QString& def,
                                 bool* ok) {
	
    // lookup:
    QVariant ret = readEntryCache(key);
    if (!ret.isValid()) {
				
        QSettings s(companyKey, appKey);
    	// RVT_PORT not supported anymore s.insertSearchPath(QSettings::Windows, companyKey);
		
		if (ok!=NULL) {
			*ok=s.contains(QString("%1%2").arg(group).arg(key));
		}
		
        ret = s.value(QString("%1%2").arg(group).arg(key), QVariant(def));

		addToCache(key, ret);
    }

    return ret.toString();

}

QByteArray RS_Settings::readByteArrayEntry(const QString& key,
                    const QString& def,
                    bool* ok) {
    QVariant ret = readEntryCache(key);
    if (!ret.isValid()) {

        QSettings s(companyKey, appKey);
        // RVT_PORT not supported anymore s.insertSearchPath(QSettings::Windows, companyKey);

                if (ok!=NULL) {
                        *ok=s.contains(QString("%1%2").arg(group).arg(key));
                }

        ret = s.value(QString("%1%2").arg(group).arg(key), QVariant(def));

                addToCache(key, ret);
    }

    return ret.toByteArray();

}

int RS_Settings::readNumEntry(const QString& key, int def, bool* ok) {

    // lookup:
    QVariant ret = readEntryCache(key);
    if (!ret.isValid()) {
        QSettings s(companyKey, appKey);
    	// RVT_PORT not supported anymore s.insertSearchPath(QSettings::Windows, companyKey);

		if (ok!=NULL) {
			*ok=s.contains(QString("%1%2").arg(group).arg(key));
		}
        ret = s.value(QString("%1%2").arg(group).arg(key), QVariant(def));
		addToCache(key, ret);
	}
	return ret.toInt();
}


QVariant RS_Settings::readEntryCache(const QString& key) {
       QVariant *s = cache.value(key);
        if (s == NULL)
            return QVariant();
        else
            return *s;
}


void RS_Settings::addToCache(const QString& key, const QVariant& value) {
        delete cache.take(key);
        cache.insert(key, new QVariant(value));
}
