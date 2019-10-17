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
#ifndef LC_TELEMETRY_H
#define LC_TELEMETRY_H

#include <QLibrary>
#include <QString>

typedef void(*BeginSessionFunc)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);
typedef void(*EndSessionFunc)();
typedef void(*TrackEventFunc)(const wchar_t*);
typedef void(*TrackMetricFunc)(const wchar_t*, const double);
typedef void(*TrackPageViewFunc)(const wchar_t*);
typedef void(*AddPropertyFunc)(const wchar_t*, const wchar_t*);
typedef void(*RemovePropertyFunc)(const wchar_t*);

class LC_Telemetry
{
public:
    LC_Telemetry();
	virtual ~LC_Telemetry();

	void BeginSession();
	void EndSession();
	void TrackEvent(const QString& eventName);
	void TrackMetric(const QString& metricName, const double value);
	void TrackPageView(const QString& pageName);
	void AddProperty(const QString& key, const QString& value);
	void RemoveProperty(const QString& key);

private:
	void ConnectFunctions();
	QString GetUserName();
	QString GetHaspId();
	QString GetProductName();
	QString GetProductVersion();
	QString GetAssociate();

private:
	QLibrary *library;
	BeginSessionFunc beginSession;
	EndSessionFunc endSession;
	TrackEventFunc trackEvent;
	TrackMetricFunc trackMetric;
	TrackPageViewFunc trackPageView;
	AddPropertyFunc addProperty;
	RemovePropertyFunc removeProperty;
	bool open;
};

struct LC_TelemetryData 
{
	int trimExcessClicks = 0;
	int textShapingClicks = 0;
	int ttfFontsConverted = 0;
	int shxFontsConverted = 0;
	int fontConversionClicks = 0;
};

#endif // LC_TELEMETRY_H
