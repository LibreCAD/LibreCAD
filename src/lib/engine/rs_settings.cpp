/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_settings.h"
#include "rs_debug.h"
#include <iostream>

RS_Settings* RS_Settings::uniqueInstance = NULL;


RS_Settings::RS_Settings() {
    initialized = false;
    companyKey = "";
    appKey = "";
    group = "";
    cache.setAutoDelete(true);
}

/**
 * Initialisation.
 *
 * @param companyKey String that identifies the company. Must start
 *        with a "/". E.g. "/RibbonSoft"
 * @param appKey String that identifies the application. Must start
 *        with a "/". E.g. "/CADuntu"
 */
void RS_Settings::init(const RS_String& companyKey,
                       const RS_String& appKey) {

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
RS_Settings::~RS_Settings() {}



void RS_Settings::beginGroup(const RS_String& group) {
    this->group = group;
}

void RS_Settings::endGroup() {
    this->group = "";
}

bool RS_Settings::writeEntry(const RS_String& key, int value) {
    return writeEntry(key, QVariant(value));
}

bool RS_Settings::writeEntry(const RS_String& key,const QString& value) {
    return writeEntry(key, QVariant(value));
}

bool RS_Settings::writeEntry(const RS_String& key, double value) {
    return writeEntry(key, QVariant(value));
}

bool RS_Settings::writeEntry(const RS_String& key, const QVariant& value) {
	QSettings s(companyKey, appKey);
    // RVT_PORT not supported anymore s.insertSearchPath(QSettings::Windows, companyKey);

    s.setValue(QString("%1%2").arg(group).arg(key), value);

	addToCache(key, value);
	
	// RVT_PORT todo, remove bool 
    return true;
}

RS_String RS_Settings::readEntry(const RS_String& key,
                                 const RS_String& def,
                                 bool* ok) {
	
    // lookup:
    QVariant ret = readEntryCache(key);
    if (ret==NULL) {
				
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

int RS_Settings::readNumEntry(const RS_String& key, int def, bool* ok) {

    // lookup:
    QVariant ret = readEntryCache(key);
    if (ret==NULL) {
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


QVariant RS_Settings::readEntryCache(const RS_String& key) {
	QVariant* s = cache.find(key);
	if (s==NULL) {
		return NULL;
	}
	else {
		return *s;
	}
}


void RS_Settings::addToCache(const RS_String& key, const QVariant& value) {
	cache.replace(key, new QVariant(value));
}
