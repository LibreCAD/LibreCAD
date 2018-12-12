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
#include "rs_settings.h"

RS_Settings* RS_Settings::uniqueInstance = nullptr;
bool RS_Settings::save_is_allowed = true;

RS_Settings::RS_Settings():
	initialized(false)
{
}

RS_Settings* RS_Settings::instance() {
	if (!uniqueInstance) {
		uniqueInstance = new RS_Settings();
	}
	return uniqueInstance;
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
	cache[key]=value;

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
		
		if (ok) {
			*ok=s.contains(QString("%1%2").arg(group).arg(key));
		}
		
        ret = s.value(QString("%1%2").arg(group).arg(key), QVariant(def));
		cache[key]=ret;
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

                if (ok) {
                        *ok=s.contains(QString("%1%2").arg(group).arg(key));
                }

        ret = s.value(QString("%1%2").arg(group).arg(key), QVariant(def));
		cache[key]=ret;
    }

    return ret.toByteArray();

}

int RS_Settings::readNumEntry(const QString& key, int def)
{
	QVariant value = readEntryCache(key);
	if (!value.isValid())
	{
		QSettings s(companyKey, appKey);
		QString str = QString("%1%2").arg(group).arg(key);
		// qDebug() << str;
		value = s.value(str, QVariant(def));
		cache[key] = value;
	}
	return value.toInt();
}


QVariant RS_Settings::readEntryCache(const QString& key) {
	if(!cache.count(key)) return QVariant();
	return cache[key];
}


void RS_Settings::addToCache(const QString& key, const QVariant& value) {
    cache[key]=value;
}

void RS_Settings::clear_all()
{
    QSettings s(companyKey, appKey);
    s.clear();
    save_is_allowed = false;
}

void RS_Settings::clear_geometry()
{
    QSettings s(companyKey, appKey);
    s.remove("/Geometry");
    save_is_allowed = false;
}
