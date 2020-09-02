/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#include "lc_telemetry.h"
#include "lc_helpbrowser.h"
#include <QFileinfo>
#include <QDir>
#include <QCoreApplication>
#include <QLocale>
#include <cstdlib>
#include <string>
#include <algorithm>
#define _X86_
#include "winreg.h"
#include "winerror.h"

LC_Telemetry* LC_Telemetry::uniqueInstance = nullptr;

LC_Telemetry * LC_Telemetry::instance()
{
	if (!uniqueInstance) {
		uniqueInstance = new LC_Telemetry();
	}
	return uniqueInstance;
}

LC_Telemetry::LC_Telemetry() : open(false)
{
	QString path = QFileInfo(QDir::cleanPath(QCoreApplication::applicationDirPath()), "ProNestUtils.dll").filePath();
	library = new QLibrary(path);
	ConnectFunctions();
}

LC_Telemetry::~LC_Telemetry()
{
	delete library;
}

void LC_Telemetry::BeginSession()
{
	if (beginSession && addProperty) {
		beginSession(
			GetUserName().toStdWString().c_str(), 
			GetHaspId().toStdWString().c_str(), 
			GetProductName().toStdWString().c_str(), 
			GetProductVersion().toStdWString().c_str()
		);
		addProperty(L"Associate", GetAssociate().toStdWString().c_str());
		addProperty(L"Locale", GetLocale().toStdWString().c_str());
		addProperty(L"Locale Name", LC_HELP->getLocaleName().toStdWString().c_str());
		open = true;
	}
}

void LC_Telemetry::EndSession()
{
	if (open && endSession) {
		endSession();
		open = false;
	}		
}

void LC_Telemetry::TrackEvent(const QString & eventName)
{
	if (open && trackEvent)
		trackEvent(eventName.toStdWString().c_str());
}

void LC_Telemetry::TrackMetric(const QString & metricName, const double value)
{
	if (open && trackMetric)
		trackMetric(metricName.toStdWString().c_str(), value);
}

void LC_Telemetry::TrackPageView(const QString & pageName)
{
	if (open && trackPageView)
		trackPageView(pageName.toStdWString().c_str());
}

void LC_Telemetry::AddProperty(const QString & key, const QString & value)
{
	if (open && addProperty)
		addProperty(key.toStdWString().c_str(), value.toStdWString().c_str());
}

void LC_Telemetry::RemoveProperty(const QString & key)
{
	if (open && removeProperty)
		removeProperty(key.toStdWString().c_str());
}

void LC_Telemetry::ConnectFunctions()
{
	if (library->load()) {
		beginSession = (BeginSessionFunc)library->resolve("BeginTelemetrySession");
		endSession = (EndSessionFunc)library->resolve("EndTelemetrySession");
		trackEvent = (TrackEventFunc)library->resolve("TrackEvent");
		trackMetric = (TrackMetricFunc)library->resolve("TrackMetric");
		trackPageView = (TrackPageViewFunc)library->resolve("TrackPageView");
		addProperty = (AddPropertyFunc)library->resolve("AddProperty");
		removeProperty = (RemovePropertyFunc)library->resolve("RemoveProperty");
	}	
}

QString LC_Telemetry::GetUserName()
{
	return QString(std::getenv("USERNAME"));
}

QString LC_Telemetry::GetHaspId()
{
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\LibreCAD\\LibreCAD\\Telemetry", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		WCHAR szBuffer[512];
		DWORD dwBufferSize = sizeof(szBuffer);
		if (RegQueryValueExW(hKey, L"HaspId", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS) {
			return QString::fromStdWString(szBuffer);
		}
	}
	return QString("");
}

QString LC_Telemetry::GetProductName()
{
	return QString("LibreCAD");
}

QString LC_Telemetry::GetProductVersion()
{
	return QString(LC_VERSION);
}

QString LC_Telemetry::GetAssociate()
{
	auto ptr = std::getenv("USERDNSDOMAIN");
	if (ptr != nullptr) {
		std::string dns = ptr;
		std::transform(dns.begin(), dns.end(), dns.begin(), toupper);
		if (dns.find("HYPERTHERM", 0) != std::string::npos)
			return QString("-1");
	}
	return QString("0");
}

QString LC_Telemetry::GetLocale()
{
	QLocale l;
	return l.name().replace('_', '-');
}
