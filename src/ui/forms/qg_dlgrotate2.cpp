/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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
#include "qg_dlgrotate2.h"

#include "rs_settings.h"
#include "rs_math.h"
#include "rs_modification.h"

/*
 *  Constructs a QG_DlgRotate2 as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgRotate2::QG_DlgRotate2(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgRotate2::~QG_DlgRotate2()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgRotate2::languageChange()
{
    retranslateUi(this);
}

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

