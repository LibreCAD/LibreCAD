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

RS_Settings::GroupGuard::GroupGuard(QString group): m_group{std::move(group)}
{}

RS_Settings::GroupGuard::~GroupGuard()
{
    try {
        RS_SETTINGS->endGroup();
        if (!m_group.isEmpty()) {
            RS_SETTINGS->beginGroup(m_group);
        }
    } catch (...) {
        LC_LOG(RS_Debug::D_CRITICAL)<<"RS_SETTINGS cleanup error";
    }
}

bool RS_Settings::save_is_allowed = true;

RS_Settings::RS_Settings():
	initialized(false)
{
}

RS_Settings* RS_Settings::instance() {
    static RS_Settings* uniqueInstance = new RS_Settings();
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

    m_group = "";
	
    this->companyKey = companyKey;
    this->appKey = appKey;

    //insertSearchPath(QSettings::Windows, companyKey + appKey);
    //insertSearchPath(QSettings::Unix, "/usr/share/");
    initialized = true;
}

void RS_Settings::beginGroup(QString group) {
    m_group = std::move(group);
}

void RS_Settings::endGroup() {
    m_group.clear();
}

std::unique_ptr<RS_Settings::GroupGuard> RS_Settings::beginGroupGuard(QString group) {
    auto guard = std::make_unique<RS_Settings::GroupGuard>(std::move(m_group));
    m_group = std::move(group);
    return guard;
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

    // Skip writing operations if the key is found in the cache and
    // its value is the same as the new one (it was already written).
    QVariant ret = readEntryCache(key);
    if (ret.isValid() && ret == value) {
        return true;
    }

	QSettings s(companyKey, appKey);
    // RVT_PORT not supported anymore s.insertSearchPath(QSettings::Windows, companyKey);

    s.setValue(QString("%1%2").arg(m_group).arg(key), value);
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
            *ok=s.contains(QString("%1%2").arg(m_group).arg(key));
		}
		
        ret = s.value(QString("%1%2").arg(m_group).arg(key), QVariant(def));
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
                        *ok=s.contains(QString("%1%2").arg(m_group).arg(key));
                }

        ret = s.value(QString("%1%2").arg(m_group).arg(key), QVariant(def));
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
        QString str = QString("%1%2").arg(m_group).arg(key);
		// qDebug() << str;
		value = s.value(str, QVariant(def));
		cache[key] = value;
	}
    unsigned long long uValue = value.toULongLong();
    uValue = uValue % 0x80000000ull;
    return int(uValue);
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
