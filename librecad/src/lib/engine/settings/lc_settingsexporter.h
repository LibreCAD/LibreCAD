/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
 ******************************************************************************/

#ifndef LC_SETTINGSEXPORTER_H
#define LC_SETTINGSEXPORTER_H

#include <QSettings>

class LC_SettingsExporter: public QObject{
  Q_OBJECT
public:
   bool exportSettings(QWidget* parent = nullptr);
   bool importSettings(QWidget* parent = nullptr);
   void exportCustomWidgetSettings(QWidget* parent, bool forMenu);
   void importCustomWidgetSettings(QWidget* parent, bool forMenu);
protected:
  bool obtainSettingsFileName(QWidget* parent, QString& fileName, bool forRead);
  bool obtainCustomWidgetsFileName(QWidget* parent, QString& fileName, bool forRead, bool forMenu);
  void exportValue(const QString& key, const QVariant& settingValue, QJsonObject& objValues);
  void exportKeyValue(const QString& key, QSettings* settings, QJsonObject &objValues);
  void exportGroup(QSettings* settings, const QString& group, QJsonObject &objGroups);
};

#endif
