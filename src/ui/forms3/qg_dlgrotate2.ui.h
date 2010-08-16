/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

void QG_DlgRotate2::init() {
    RS_SETTINGS->beginGroup("/Modify");
    copies = RS_SETTINGS->readEntry("/Rotate2Copies", "10");
    numberMode = RS_SETTINGS->readNumEntry("/Rotate2Mode", 0);
    useCurrentLayer =
        (bool)RS_SETTINGS->readNumEntry("/Rotate2UseCurrentLayer", 0);
    useCurrentAttributes =
        (bool)RS_SETTINGS->readNumEntry("/MoveRotate2UseCurrentAttributes", 0);
    angle1 = RS_SETTINGS->readEntry("/Rotate2Angle1", "30.0");
    angle2 = RS_SETTINGS->readEntry("/Rotate2Angle2", "-30.0");
    RS_SETTINGS->endGroup();

    switch (numberMode) {
    case 0:
        rbMove->setChecked(true);
        break;
    case 1:
        rbCopy->setChecked(true);
        break;
    case 2:
        rbMultiCopy->setChecked(true);
        break;
    default:
        break;
    }
    leNumber->setText(copies);
    leAngle1->setText(angle1);
    leAngle2->setText(angle2);
    cbCurrentAttributes->setChecked(useCurrentAttributes);
    cbCurrentLayer->setChecked(useCurrentLayer);
}

void QG_DlgRotate2::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/Rotate2Copies", leNumber->text());
    if (rbMove->isChecked()) {
        numberMode = 0;
    } else if (rbCopy->isChecked()) {
        numberMode = 1;
    } else {
        numberMode = 2;
    }
    RS_SETTINGS->writeEntry("/Rotate2Mode", numberMode);
    RS_SETTINGS->writeEntry("/Rotate2Angle1", leAngle1->text());
    RS_SETTINGS->writeEntry("/Rotate2Angle2", leAngle2->text());
    RS_SETTINGS->writeEntry("/Rotate2UseCurrentLayer",
                            (int)cbCurrentLayer->isChecked());
    RS_SETTINGS->writeEntry("/Rotate2UseCurrentAttributes",
                            (int)cbCurrentAttributes->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_DlgRotate2::setData(RS_Rotate2Data* d) {
    data = d;

    //leAngle1->setText(QString("%1").arg(RS_Math::rad2deg(data->angle1)));
    //leAngle2->setText(QString("%1").arg(RS_Math::rad2deg(data->angle2)));
}

void QG_DlgRotate2::updateData() {
    if (rbMove->isChecked()) {
        data->number = 0;
    } else if (rbCopy->isChecked()) {
        data->number = 1;
    } else {
        data->number = leNumber->text().toInt();
    }
    data->angle1 = RS_Math::deg2rad(RS_Math::eval(leAngle1->text()));
    data->angle2 = RS_Math::deg2rad(RS_Math::eval(leAngle2->text()));
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}

