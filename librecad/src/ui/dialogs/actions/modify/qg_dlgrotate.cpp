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
#include<cmath>
#include "qg_dlgrotate.h"

#include "rs_settings.h"
#include "rs_math.h"
#include "rs_modification.h"

/*
 *  Constructs a QG_DlgRotate as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgRotate::QG_DlgRotate(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgRotate::~QG_DlgRotate()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgRotate::languageChange()
{
    retranslateUi(this);
}

void QG_DlgRotate::init() {
    LC_GROUP_GUARD("Modify");
    {
        copies = LC_GET_STR("RotateCopies", "10");
        numberMode = LC_GET_INT("RotateMode", 0);
        angle = LC_GET_STR("RotateAngle", "90.0");
        useCurrentLayer = LC_GET_BOOL("RotateUseCurrentLayer");
        useCurrentAttributes = LC_GET_BOOL("RotateUseCurrentAttributes");
    }

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
    LC_GROUP_GUARD("Modify");
    {
        LC_SET("RotateCopies", leNumber->text());
        if (rbMove->isChecked()) {
            numberMode = 0;
        } else if (rbCopy->isChecked()) {
            numberMode = 1;
        } else {
            numberMode = 2;
        }
        LC_SET("RotateMode", numberMode);
        LC_SET("RotateAngle", leAngle->text());
        LC_SET("RotateUseCurrentLayer", cbCurrentLayer->isChecked());
        LC_SET("RotateUseCurrentAttributes", cbCurrentAttributes->isChecked());
    }
}

void QG_DlgRotate::setData(RS_RotateData* d) {
    data = d;
    if( fabs(data->angle) > RS_TOLERANCE ) {
        angle=QString::number(RS_Math::rad2deg(data->angle));
    }
    leAngle->setText(angle);

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
