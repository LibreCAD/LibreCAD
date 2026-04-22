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

#include "lc_settingsexporter.h"

#include <QFileDialog>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QSettings>

#include "lc_filenameselectionservice.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {
    const QString G_SETTINGS_FILE_TYPE = "LibreCAD_SETTINGS";
    const QString G_CUSTOM_MENUS_FILE_TYPE = "LibreCAD_CUSTOM_MENUS";
    const QString G_CUSTOM_TOOLBARS_FILE_TYPE = "LibreCAD_CUSTOM_TOOLBARS";
    const QString G_KEY_FILE_TYPE = "_lc_file_type";
    const QString G_STRING_LIST_SEPARATOR = "|";
    const QString G_STRING_LIST_VALUE_START = "SL$";
}

bool LC_SettingsExporter::obtainSettingsFileName(QWidget* parent, QString& fileName, const bool forRead) {
    return LC_FileNameSelectionService::doObtainFileName(parent, fileName, forRead, "lcs", "lc_settings", tr("Import settings"),
                                                         tr("Export Settings"), tr("LibreCAD settings file (*.%1)"));
}

bool LC_SettingsExporter::obtainCustomWidgetsFileName(QWidget* parent, QString& fileName, const bool forRead, const bool forMenu) {
    if (forMenu) {
        return LC_FileNameSelectionService::doObtainFileName(parent, fileName, forRead, "lcm", "lc_custom_menus.lcm",
                                                             tr("Import custom menus"), tr("Export custom menus"),
                                                             tr("LibreCAD custom menus file (*.%1)"));
    }
    return LC_FileNameSelectionService::doObtainFileName(parent, fileName, forRead, "lct", "lc_custom_toolbars.lcm",
                                                         tr("Import custom toolbars"), tr("Export custom toolbars"),
                                                         tr("LibreCAD custom toolbars file (*.%1)"));
}

void LC_SettingsExporter::exportValue(const QString& key, const QVariant& settingValue, QJsonObject& objValues) {
    QString value;
    if (settingValue.userType() ==  QMetaType::QStringList) {
        QStringList stringList = settingValue.toStringList();
        value += G_STRING_LIST_VALUE_START;
        for (const auto& s: std::as_const(stringList)) {
            value += G_STRING_LIST_SEPARATOR;
            value += s;
        }
    }
    else {
        value = settingValue.toString();
    }
    objValues.insert(key, QJsonValue::fromVariant(value));
}

void LC_SettingsExporter::exportKeyValue(const QString& key, QSettings* settings, QJsonObject &objValues) {
    const QVariant settingValue = settings->value(key);
    exportValue(key, settingValue, objValues);
}

void LC_SettingsExporter::exportGroup(QSettings* settings, const QString& group, QJsonObject &objGroups) {
    settings->beginGroup(group);
    QJsonObject objValues;
    for (const QString& key : settings->childKeys()) {
        exportKeyValue(key, settings, objValues);
    }
    objGroups.insert(group, objValues);
    settings->endGroup();
}

bool LC_SettingsExporter::exportSettings(QWidget* parent) {
    QString fileName;
    if (!obtainSettingsFileName(parent, fileName, false)) {
        // file dialog cancelled
        return false;
    }

    QJsonObject objGroups;
    QSettings* settings = RS_SETTINGS->getSettings();
    for (const QString& group : settings->childGroups()) {
        exportGroup(settings, group, objGroups);
    }

    QJsonObject objSettings;
    objSettings.insert(G_KEY_FILE_TYPE, QJsonValue::fromVariant(G_SETTINGS_FILE_TYPE));
    objSettings.insert("groups", objGroups);
    const QJsonDocument doc(objSettings);
    QFile jsonFile{fileName};
    if (!jsonFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(parent, tr("Settings Export"),
                              tr(
                                  "Can't open provided file for writing - check that provided location is writable. Preferences were not exported."));
        return false;
    }
    jsonFile.write(doc.toJson());

    LC_GROUP_GUARD("Export");
    {
        LC_SET("ExportSettingsDir", QFileInfo(jsonFile).absolutePath());
    }
    QMessageBox::information(parent, tr("Settings Export"), tr("Application preferences were exported."));
    return true;
}

bool LC_SettingsExporter::importSettings(QWidget* parent) {
    QString fileName;
    if (!obtainSettingsFileName(parent, fileName, true)) {
        return false;
    }
    auto jsonFile = QFile(fileName);
    if (!jsonFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(parent, tr("Settings Import Error"),
                              tr("Can't open provided file for reading. Preferences were not imported."));
        return false;
    }
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(parent, tr("Settings Import Error"),
                              tr("Unexpected error during preferences parsing. Message:") + parseError.errorString());
        return false;
    }
    if (!doc.isObject() || doc.object().value(G_KEY_FILE_TYPE).toString() != G_SETTINGS_FILE_TYPE) {
        QMessageBox::critical(parent, tr("Settings Import Error"),
                              tr("Unexpected format of file, it does not contains LibreCAD preferences."));
        return false;
    }
    const QJsonObject& groups = doc.object().value("groups").toObject();
    if (groups.empty()) {
        QMessageBox::information(parent, tr("Settings Import"), tr("No settings groups to import."));
        return false;
    }
    for (const QString& groupName : groups.keys()) {
        const QJsonObject& groupObj = groups.value(groupName).toObject();
        if (groupObj.empty()) {
            continue;
        }

        LC_GROUP(groupName);
        {
            for (const QString& propertyName : groupObj.keys()) {
                auto jsonValue = groupObj.value(propertyName).toString();
                if (jsonValue.startsWith(G_STRING_LIST_VALUE_START)) {
                    auto parts = jsonValue.split(G_STRING_LIST_SEPARATOR, Qt::KeepEmptyParts);
                    const int count = parts.count();
                    QStringList valuesList;
                    for (int i = 1; i < count; i++) { // skip fist part which is list marker
                         valuesList.append(parts.at(i));
                    }
                    LC_SET(propertyName, valuesList);
                }
                else {
                    LC_SET(propertyName, jsonValue);
                }
            }
        }
    }

    QMessageBox::information(parent, tr("Settings Import"), tr("Application preferences were imported."));
    return true;
}

void LC_SettingsExporter::importCustomWidgetSettings(QWidget* parent, bool forMenu) {
      QString fileName;
    if (!obtainCustomWidgetsFileName(parent, fileName, true, forMenu)) {
        return;
    }
    auto jsonFile = QFile(fileName);
    if (!jsonFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(parent, tr("Configurations Import Error"),
                              tr("Can't open provided file for reading. Configurations were not imported."));
        return;
    }
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(parent, tr("Configurations Import Error"),
                              tr("Unexpected error during configurations file parsing. Message:") + parseError.errorString());
        return;
    }
    if (!doc.isObject() || doc.object().value(G_KEY_FILE_TYPE).toString() != (forMenu? G_CUSTOM_MENUS_FILE_TYPE : G_CUSTOM_TOOLBARS_FILE_TYPE)) {
        QMessageBox::critical(parent, tr("Configurations Import Error"),
                              tr("Unexpected format of file, it does not contains proper LibreCAD data."));
        return;
    }
    const QJsonObject& groups = doc.object().value(forMenu ? "menus" : "toolbars").toObject();
    if (groups.empty()) {
        QMessageBox::information(parent, tr("Configurations Import"), tr("No items to import."));
        return;
    }
    for (const QString& groupName : groups.keys()) {
        const QJsonObject& groupObj = groups.value(groupName).toObject();
        if (groupObj.empty()) {
            continue;
        }

        LC_GROUP(groupName);
        {
            for (const QString& propertyName : groupObj.keys()) {
                auto jsonValue = groupObj.value(propertyName).toString();
                if (jsonValue.startsWith(G_STRING_LIST_VALUE_START)) {
                    auto parts = jsonValue.split(G_STRING_LIST_SEPARATOR, Qt::KeepEmptyParts);
                    const int count = parts.count();
                    QStringList valuesList;
                    for (int i = 1; i < count; i++) { // skip fist part which is list marker
                         valuesList.append(parts.at(i));
                    }
                    LC_SET(propertyName, valuesList);
                }
                else {
                    LC_SET(propertyName, jsonValue);
                }
            }
        }
    }

    QMessageBox::information(parent, tr("Configurations Import"), tr("Configurations were imported."));
}

void LC_SettingsExporter::exportCustomWidgetSettings(QWidget* parent, bool forMenu) {
    QString fileName;
    if (!obtainCustomWidgetsFileName(parent, fileName, false, forMenu)) {
        // file dialog cancelled
        return;
    }

    QJsonObject objGroups;
    QSettings* settings = RS_SETTINGS->getSettings();
    const QString group = forMenu ? "CustomMenus" : "CustomToolbars";
    exportGroup(settings, group, objGroups);
    if (forMenu) {
        settings->beginGroup(group);
        QSet<QString> menusKeys;
        for (const QString& key : settings->childKeys()) {
            menusKeys.insert(key);
        }

        settings->endGroup();
        settings->beginGroup("Activators");

        QJsonObject activatorsGroup;
        for (const QString& key : settings->childKeys()) {
            QString value = settings->value(key).toString();
            if (menusKeys.contains(value)) {
                exportValue(key,value, activatorsGroup);
            }
        }
        objGroups.insert("Activators", activatorsGroup);
        settings->endGroup();
    }

    QJsonObject objSettings;
    objSettings.insert(G_KEY_FILE_TYPE, QJsonValue::fromVariant(forMenu ? G_CUSTOM_MENUS_FILE_TYPE : G_CUSTOM_TOOLBARS_FILE_TYPE));
    objSettings.insert(forMenu ? "menus" : "toolbars", objGroups);
    const QJsonDocument doc(objSettings);
    QFile jsonFile{fileName};
    if (!jsonFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(parent, forMenu ? tr("Custom Menus Export") : tr("Custom Toolbars Export"),
                              tr(
                                  "Can't open provided file for writing - check that provided location is writable. Export was not exported."));
        return;
    }
    jsonFile.write(doc.toJson());

    LC_GROUP_GUARD("Export");
    {
        LC_SET("ExportWidgetSettingsDir", QFileInfo(jsonFile).absolutePath());
    }
    if (forMenu) {
        QMessageBox::information(parent, tr("Custom Menus Export"), tr("Custom menu configurations were exported."));
    }
    else {
        QMessageBox::information(parent, tr("Custom Toolbars Export"), tr("Custom toolbars configurations were exported."));
    }
}
