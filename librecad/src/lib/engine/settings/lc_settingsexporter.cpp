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
#include <QMessageBox>
#include <QSettings>

#include "rs_debug.h"
#include "rs_settings.h"
#include "rs_system.h"

bool LC_SettingsExporter::obtainFileName(QWidget *parent, QString &fileName, bool forRead){
    LC_GROUP("Export");
    QString defDir = LC_GET_STR("ExportSettingsDir", RS_SYSTEM->getHomeDir());
    LC_GROUP_END();

    bool useQtFileDialog = LC_GET_ONE_BOOL("Defaults","UseQtFileOpenDialog");

    QFileDialog fileDlg(parent, forRead ? tr("Import settings")  : tr("Export Settings"));

    fileDlg.setNameFilters(QStringList("*.lcs"));
    fileDlg.setFileMode(forRead ? QFileDialog::ExistingFile : QFileDialog::AnyFile);
    fileDlg.selectNameFilter("*.lcs");
    fileDlg.setAcceptMode(forRead ? QFileDialog::AcceptOpen : QFileDialog::AcceptSave);
    fileDlg.setOption(QFileDialog::DontUseNativeDialog, useQtFileDialog);
    fileDlg.setDirectory(defDir);
    fileDlg.selectFile("LCSettings");

    bool proceed = false;
    if (fileDlg.exec() == QDialog::Accepted) {
        QStringList files = fileDlg.selectedFiles();
        if (!files.isEmpty()) {
            fileName = files[0];

            if (!fileName.endsWith(".lcs")) {
                fileName = fileName + ".lcs";
            }
            proceed = true;
        }
    }
    return proceed;
}

bool LC_SettingsExporter::exportSettings(QWidget* parent){
    bool result = false;
    QString fileName;
    if (obtainFileName(parent, fileName, false)) {
        QJsonObject objSettings;
        objSettings.insert("type", QJsonValue::fromVariant("LibreCAD settings file"));
        QJsonObject objGroups;

        auto settings = RS_SETTINGS->getSettings();
        auto groups = settings->childGroups();
        for (const auto& group: groups) {
            settings->beginGroup(group);
            QJsonObject objValues;
            auto keys = settings->childKeys();
            for (const auto& key: keys) {
                QString value = settings->value(key).toString();
                objValues.insert(key, QJsonValue::fromVariant(value));
            }
            objGroups.insert(group, objValues);
            settings->endGroup();
        }

        objSettings.insert("groups", objGroups);

        QJsonDocument doc(objSettings);
        QFile jsonFile = QFile(fileName);
        bool canWrite = jsonFile.open(QFile::WriteOnly);
        if (canWrite) {
            jsonFile.write(doc.toJson());

            LC_GROUP_GUARD("Export"); {
                LC_SET("ExportSettingsDir", QFileInfo(jsonFile).absolutePath());
            }
           QMessageBox::information(parent, tr("Settings Export"), tr("Application preferences were exported."));
           result = true;
        }
        else {
            QMessageBox::critical(parent, tr("Settings Export"), tr("Can't open provided file for writing - check that provided location is writable. Preferences were not exported."));
        }
    }
    return result;
}

bool LC_SettingsExporter::importSettings(QWidget *parent) {
    bool result = false;
    QString fileName;
    if (obtainFileName(parent, fileName, true)) {
        QFile jsonFile = QFile(fileName);
        if (jsonFile.open(QFile::ReadOnly)) {
            QJsonParseError parseError;
            auto doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                QMessageBox::critical(parent, tr("Settings Import Error"), tr("Unexpected error during preferences parsing. Message:") + parseError.errorString());
            }
            else{
                bool canParse;
                QJsonObject obj;
                if (doc.isObject()) {
                    obj = doc.object();
                    auto type = obj.value("type").toString();
                    canParse = "LibreCAD settings file" == type;
                } else {
                    canParse = false;
                }
                if (canParse) {
                    QJsonObject objGroups = obj.value("groups").toObject();
                    QJsonObject::const_iterator group = objGroups.constBegin();
                    QJsonObject::const_iterator groupEnd = objGroups.constEnd();
                    if (group != groupEnd) {
                        do {
                            auto groupName = group.key();
                            auto groupObj = group.value().toObject();

                            QJsonObject::const_iterator property = groupObj.constBegin();
                            QJsonObject::const_iterator propertyEnd = groupObj.constEnd();

                            if (property != propertyEnd) {
//                                LC_ERR << "----------- Group: " << groupName;

                                LC_GROUP(groupName);
                                {
                                    do {
                                        auto propertyName = property.key();
                                        auto propertyValue = property.value().toString();

//                                        LC_ERR << "Property: " << propertyName << "  Value: " << propertyValue;
                                        LC_SET(propertyName, propertyValue);

                                        property++;
                                    }
                                    while (property != propertyEnd);
                                }
                                LC_GROUP_END();
                            }
                            group++;
                        } while (group != groupEnd);
                        QMessageBox::information(parent, tr("Settings Import"), tr("Application preferences were imported."));
                        result = true;
                    }
                    else {
                        QMessageBox::information(parent, tr("Settings Import"), tr("No settings groups to import."));
                        result = true;
                    }

                } else {
                    QMessageBox::critical(parent, tr("Settings Import Error"), tr("Unexpected format of file, it does not contains LibreCAD preferences."));
                }
            }
        } else {
            QMessageBox::critical(parent, tr("Settings Import Error"), tr("Can't open provided file for reading. Preferences were not imported."));
        }
    }
    return result;
}
