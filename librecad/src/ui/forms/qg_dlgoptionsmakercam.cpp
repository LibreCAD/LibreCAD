/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**
**********************************************************************/

#include "qg_dlgoptionsmakercam.h"

#include "rs_settings.h"

QG_DlgOptionsMakerCam::QG_DlgOptionsMakerCam(QWidget* parent, bool modal, Qt::WindowFlags fl) : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    loadSettings();
}

void QG_DlgOptionsMakerCam::languageChange()
{
    retranslateUi(this);
}

void QG_DlgOptionsMakerCam::validate() {

    saveSettings();

    accept();
}

void QG_DlgOptionsMakerCam::cancel() {
    reject();
}

void QG_DlgOptionsMakerCam::loadSettings() {

    RS_SETTINGS->beginGroup("/ExportMakerCam");

    updateCheckbox(checkInvisibleLayers, "ExportInvisibleLayers", 0);
    updateCheckbox(checkConstructionLayers, "ExportConstructionLayers", 0);
    updateCheckbox(checkBlocksInline, "WriteBlocksInline", 1);
    updateCheckbox(checkEllipsesToBeziers, "ConvertEllipsesToBeziers", 1);

    RS_SETTINGS->endGroup();
}

void QG_DlgOptionsMakerCam::updateCheckbox(QCheckBox* checkbox, QString name, int defaultValue) {

    checkbox->setChecked(RS_SETTINGS->readNumEntry("/" + name, defaultValue) ? true : false);
}

void QG_DlgOptionsMakerCam::saveSettings() {

    RS_SETTINGS->beginGroup("/ExportMakerCam");

    saveBoolean("ExportInvisibleLayers", checkInvisibleLayers);
    saveBoolean("ExportConstructionLayers", checkConstructionLayers);
    saveBoolean("WriteBlocksInline", checkBlocksInline);
    saveBoolean("ConvertEllipsesToBeziers", checkEllipsesToBeziers);

    RS_SETTINGS->endGroup();
}

void QG_DlgOptionsMakerCam::saveBoolean(QString name, QCheckBox* checkbox) {

    RS_SETTINGS->writeEntry("/" + name, checkbox->isChecked() ? 1 : 0);
}
