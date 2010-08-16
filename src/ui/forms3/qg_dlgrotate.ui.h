/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

void QG_DlgRotate::init() {
    RS_SETTINGS->beginGroup("/Modify");
    copies = RS_SETTINGS->readEntry("/RotateCopies", "10");
    numberMode = RS_SETTINGS->readNumEntry("/RotateMode", 0);
    angle = RS_SETTINGS->readEntry("/RotateAngle", "90.0");
    useCurrentLayer =
        (bool)RS_SETTINGS->readNumEntry("/RotateUseCurrentLayer", 0);
    useCurrentAttributes =
        (bool)RS_SETTINGS->readNumEntry("/RotateUseCurrentAttributes", 0);
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
    leAngle->setText(angle);
    cbCurrentAttributes->setChecked(useCurrentAttributes);
    cbCurrentLayer->setChecked(useCurrentLayer);
}

void QG_DlgRotate::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/RotateCopies", leNumber->text());
    if (rbMove->isChecked()) {
        numberMode = 0;
    } else if (rbCopy->isChecked()) {
        numberMode = 1;
    } else {
        numberMode = 2;
    }
    RS_SETTINGS->writeEntry("/RotateMode", numberMode);
    RS_SETTINGS->writeEntry("/RotateAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/RotateUseCurrentLayer",
                            (int)cbCurrentLayer->isChecked());
    RS_SETTINGS->writeEntry("/RotateUseCurrentAttributes",
                            (int)cbCurrentAttributes->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_DlgRotate::setData(RS_RotateData* d) {
    data = d;
}

void QG_DlgRotate::updateData() {
    if (rbMove->isChecked()) {
        data->number = 0;
    } else if (rbCopy->isChecked()) {
        data->number = 1;
    } else {
        data->number = leNumber->text().toInt();
    }
    data->angle = RS_Math::deg2rad(RS_Math::eval(leAngle->text()));
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}

