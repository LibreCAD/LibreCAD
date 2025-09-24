/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_dimstylesexporter.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "lc_dimstyletovariablesmapper.h"
#include "lc_dimstyleslistmodel.h"
#include "lc_filenameselectionservice.h"

#include "rs_settings.h"
#include "lc_dimstyleitem.h"

LC_DimStylesExporter::LC_DimStylesExporter() {}


namespace{
    const QString G_DIMSTYLES_FILE_TYPE = "LibreCAD dimstyles file";
    const QString G_KEY_FILE_TYPE = "_lc_file_type";
}

bool LC_DimStylesExporter::exportStyles(QWidget* parent, const QList<LC_DimStyleItem*>& styles, const QString& baseFileName) {
    QString fileName;
    if (!obtainFileName(parent, fileName, false, baseFileName)) {  // file dialog cancelled
        return false;
    }

    QJsonArray objStyles;

    QJsonObject objExport;

    LC_DimStyleToVariablesMapper dimstyleToVarDictMapper;

    for (const LC_DimStyleItem* dimStyleItem : styles) {
       auto varDict = new RS_VariableDict();

       LC_DimStyle* dimStyle = dimStyleItem->dimStyle();
       bool basestyle = dimStyleItem->isBaseStyle();
       LC_DimStyle::ModificationAware::CheckFlagMode savedModifyCheckMode = dimStyle->arrowhead()->getModifyCheckMode();

       LC_DimStyle::ModificationAware::CheckFlagMode exportModifyCheckMode;
       if (basestyle) {
           exportModifyCheckMode = LC_DimStyle::ModificationAware::ALL;
       }
       else {
           exportModifyCheckMode = LC_DimStyle::ModificationAware::SET;
       }

       dimStyle->setModifyCheckMode(exportModifyCheckMode);
       dimstyleToVarDictMapper.toDictionary(dimStyle, varDict);
       dimStyle->setModifyCheckMode(savedModifyCheckMode);

       QJsonArray objStyleVars;

       QHashIterator<QString, RS_Variable> it(varDict->getVariableDict());

        while (it.hasNext()) {
            it.next();
            QString varName = it.key();
            RS_Variable variable = it.value();
            QJsonObject objSingleVar;
            objSingleVar.insert("type", variable.getType());
            objSingleVar.insert("val", variable.toString());
            objSingleVar.insert("code", variable.getCode());
            objSingleVar.insert("name", varName);
            objStyleVars.append(objSingleVar);
        }

       QJsonObject objStyle;
       objStyle.insert("style_name", dimStyle->getName());
       objStyle.insert("vars", objStyleVars);
       objStyles.append(objStyle);
       delete varDict;
    }
    objExport.insert("styles", objStyles);
    objExport.insert(G_KEY_FILE_TYPE, QJsonValue::fromVariant(G_DIMSTYLES_FILE_TYPE));

    QJsonDocument doc(objExport);
    QFile jsonFile{fileName};
    if (!jsonFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(parent, tr("Dimension Styles Export Error"),
            tr("Can't open provided file for writing - check that provided location is writable. Dimension styles were not exported."));
        return false;
    }
    jsonFile.write(doc.toJson());

    LC_GROUP_GUARD("Export"); {
        LC_SET("ExportSettingsDir", QFileInfo(jsonFile).absolutePath());
    }

    QMessageBox::information(parent, tr("Dimension Styles Export"), tr("Dimensions Styles were exported."));
    return true;
}

bool LC_DimStylesExporter::importStyles(QWidget* parent, QList<LC_DimStyle*>& styleItems) {
    QString fileName;
    if (!obtainFileName(parent, fileName, true, ""))
        return false;

    QFile jsonFile = QFile(fileName);
    auto errorDialogCaption = tr("Dimension Styles Import Error");
    if (!jsonFile.open(QFile::ReadOnly)) {
        QMessageBox::critical(parent, errorDialogCaption, tr("Can't open provided file for reading. Dimension styles were not imported."));
        return false;
    }
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(parent, errorDialogCaption, tr("Unexpected error during dimension styles parsing. Message:") + parseError.errorString());
        return false;
    }
    if (!doc.isObject() || doc.object().value(G_KEY_FILE_TYPE).toString() != G_DIMSTYLES_FILE_TYPE) {
        QMessageBox::critical(parent, errorDialogCaption, tr("Unexpected format of file, it does not contains LibreCAD dimension styles."));
        return false;
    }

    const QJsonArray& styles = doc.object().value("styles").toArray();
    if (styles.empty()) {
        QMessageBox::information(parent, tr("Dimension Styles Import"), tr("No dimension styles to import."));
        return false;
    }

    LC_DimStyleToVariablesMapper dimstyleToVarDictMapper;

    for (auto s: styles) {
        auto style = s.toObject();
        QString styleName = style.value("style_name").toString();
        auto varDict = new RS_VariableDict();
        auto values = style.value("vars").toArray();
        for (auto v: values) {
            auto variable = v.toObject();
            int type = variable.value("type").toInt();
            QString value = variable.value("val").toString();
            int code = variable.value("code").toInt();
            QString varName = variable.value("name").toString();

            varDict->add(varName, value, code, type);
        }

        auto* dimStyle = new LC_DimStyle();
        dimStyle->setName(styleName);

        // fixme - sand - review and decide which unit should be used for export/import of double vars
        // fixme - sand - !!! how changing of drawing unit affects dim styles?????
        dimstyleToVarDictMapper.fromDictionary(dimStyle, varDict, RS2::Unit::Millimeter);

        styleItems.append(dimStyle);
        delete varDict;
    }

    QMessageBox::information(parent, tr("Dimension Styles Import"), tr("Dimension styles were imported."));
    return true;
}


bool LC_DimStylesExporter::obtainFileName(QWidget* parent, QString& fileName, bool forRead, const QString& baseFileName) {
    QString defaultFileName = forRead ? "LC_DimStyles" : baseFileName;
    return LC_FileNameSelectionService::doObtainFileName(parent, fileName, forRead, "lcds",
        defaultFileName, tr("Import Dimension Styles"),  tr("Export Dimension Styles"),
        tr("LibreCAD dimension styles file (*.%1)"));
}
