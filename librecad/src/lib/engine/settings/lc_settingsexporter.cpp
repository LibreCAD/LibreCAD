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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QSettings>

#include "lc_filenameselectionservice.h"
#include "rs_settings.h"
#include "rs_system.h"


namespace{
    const QString G_SETTINGS_FILE_TYPE = "LibreCAD settings file";
    const QString G_KEY_FILE_TYPE = "_lc_file_type";
}

bool LC_SettingsExporter::obtainFileName(QWidget *parent, QString &fileName, bool forRead){
    return LC_FileNameSelectionService::doObtainFileName(parent, fileName, forRead, "lcs",
            "LCSettings", tr("Import settings"),  tr("Export Settings"),
            tr("LibreCAD settings file (*.%1)"));
}

bool LC_SettingsExporter::exportSettings(QWidget* parent){
    QString fileName;
    if (!obtainFileName(parent, fileName, false)) { // file dialog cancelled
        return false;
    }

    QJsonObject objGroups;
    QSettings* settings = RS_SETTINGS->getSettings();
    for (const QString& group: settings->childGroups()) {
        settings->beginGroup(group);
        QJsonObject objValues;
        for (const QString& key: settings->childKeys()) {
            QString value = settings->value(key).toString();
            objValues.insert(key, QJsonValue::fromVariant(value));
        }
        objGroups.insert(group, objValues);
        settings->endGroup();
    }

    QJsonObject objSettings;
    objSettings.insert(G_KEY_FILE_TYPE, QJsonValue::fromVariant(G_SETTINGS_FILE_TYPE));
    objSettings.insert("groups", objGroups);
    QJsonDocument doc(objSettings);
    QFile jsonFile{fileName};
    if (!jsonFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(parent, tr("Settings Export"), tr("Can't open provided file for writing - check that provided location is writable. Preferences were not exported."));
        return false;
    }
    jsonFile.write(doc.toJson());

    LC_GROUP_GUARD("Export"); {
        LC_SET("ExportSettingsDir", QFileInfo(jsonFile).absolutePath());
    }
    QMessageBox::information(parent, tr("Settings Export"), tr("Application preferences were exported."));
    return true;
}

bool LC_SettingsExporter::importSettings(QWidget *parent) {
    QString fileName;
    if (!obtainFileName(parent, fileName, true))
        return false;
    QFile jsonFile = QFile(fileName);
    if (!jsonFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(parent, tr("Settings Import Error"), tr("Can't open provided file for reading. Preferences were not imported."));
        return false;
    }
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(parent, tr("Settings Import Error"), tr("Unexpected error during preferences parsing. Message:") + parseError.errorString());
        return false;
    }
    if (!doc.isObject() || doc.object().value(G_KEY_FILE_TYPE).toString() != G_SETTINGS_FILE_TYPE) {
        QMessageBox::critical(parent, tr("Settings Import Error"), tr("Unexpected format of file, it does not contains LibreCAD preferences."));
        return false;
    }
    const QJsonObject& groups = doc.object().value("groups").toObject();
    if (groups.empty()) {
        QMessageBox::information(parent, tr("Settings Import"), tr("No settings groups to import."));
        return false;
    }
    for(const QString& groupName: groups.keys()) {
        const QJsonObject& groupObj = groups.value(groupName).toObject();
        if (groupObj.empty())
            continue;

        LC_GROUP(groupName);
        {
            for(const QString& propertyName: groupObj.keys())
                LC_SET(propertyName, groupObj.value(propertyName).toString());
        }
    }

    QMessageBox::information(parent, tr("Settings Import"), tr("Application preferences were imported."));
    return true;
}
