/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
** Copyright (C) 2018 Andrey Yaromenok (ayaromenok@gmail.com)
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
    this->gbLayers->setToolTip(tr("MakerCAM as of November 2014 does not hide SVG content \nthat has been set invisibe (\"display: none\" or \"visibility: hidden\")."));
    this->gbBlocks->setToolTip(tr("MakerCAM as of November 2014 cannot correctly deal with blocks,\nbecause it does not take into account the reference point in the <use>."));
    this->gbEllipses->setToolTip(tr("MakerCAM as of March 2015 cannot display ellipses and ellipse arcs correctly, \nwhen they are created using the <ellipse> tag  with a rotation in \nthe <transform> attribute or as <path> using elliptic arc segments."));
    this->gbImages->setToolTip(tr("Exported images can be useful in SVG editors (Inkscape, etc), \nbut avoided in some CAM's."));
    this->gbDashLines->setToolTip(tr("Many CAM's(MakerCAM, EleskCAM, LaserWeb) ignore dashed/doted line style, \nwhich can be useful in lasercut of plywood or for papercraft. "));
    this->dSpinBoxDefaultElementWidth->setToolTip(tr("Default width of elements can affect some CAM's/SVG Editors, \nbut ignored by other"));
    this->dSpinBoxDashLinePatternLength->setToolTip(tr("Length of line pattern related to zoom, \nso default step value required for baking"));

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
    updateCheckbox(checkImages, "ExportImages", 0);
    updateCheckbox(checkDashDotLines, "BakeDashDotLines", 0);
    updateDoubleSpinBox(dSpinBoxDefaultElementWidth, "DefaultElementWidth", 1.0);
    updateDoubleSpinBox(dSpinBoxDashLinePatternLength, "DefaultDashLinePatternLength", 2.5);
    RS_SETTINGS->endGroup();
}

void QG_DlgOptionsMakerCam::updateCheckbox(QCheckBox* checkbox, QString name, int defaultValue) {

    checkbox->setChecked(RS_SETTINGS->readNumEntry("/" + name, defaultValue) ? true : false);
}

void QG_DlgOptionsMakerCam::updateDoubleSpinBox(QDoubleSpinBox* dSpinBox, QString name, double defaultValue) {

    dSpinBox->setValue(RS_SETTINGS->readEntry("/" + name, QString::number(defaultValue)).toDouble());
}

void QG_DlgOptionsMakerCam::saveSettings() {

    RS_SETTINGS->beginGroup("/ExportMakerCam");

    saveBoolean("ExportInvisibleLayers", checkInvisibleLayers);
    saveBoolean("ExportConstructionLayers", checkConstructionLayers);
    saveBoolean("WriteBlocksInline", checkBlocksInline);
    saveBoolean("ConvertEllipsesToBeziers", checkEllipsesToBeziers);
    saveBoolean("ExportImages", checkImages);
    saveBoolean("BakeDashDotLines", checkDashDotLines);
    saveDouble("DefaultElementWidth", dSpinBoxDefaultElementWidth);
    saveDouble("DefaultDashLinePatternLength", dSpinBoxDashLinePatternLength);

    RS_SETTINGS->endGroup();
}

void QG_DlgOptionsMakerCam::saveBoolean(QString name, QCheckBox* checkbox) {

    RS_SETTINGS->writeEntry("/" + name, checkbox->isChecked() ? 1 : 0);
}

void QG_DlgOptionsMakerCam::saveDouble(QString name, QDoubleSpinBox* dSpinBox) {
    RS_SETTINGS->writeEntry("/" + name, dSpinBox->value());
}
