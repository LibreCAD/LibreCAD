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
#include "qg_dlgmoverotate.h"

#include "rs_settings.h"
#include "rs_math.h"
#include "rs_modification.h"

/*
 *  Constructs a QG_DlgMoveRotate as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgMoveRotate::QG_DlgMoveRotate(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgMoveRotate::~QG_DlgMoveRotate(){
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgMoveRotate::languageChange(){
    retranslateUi(this);
}

void QG_DlgMoveRotate::init() {
    rbMove->setChecked(!data->keepOriginals);
    rbCopy->setChecked(data->keepOriginals);
    cbMultiCopy->setChecked(data->multipleCopies);
    sbNumber->setValue(data->number);
    cbCurrentAttributes->setChecked(data->useCurrentAttributes);
    cbCurrentLayer->setChecked(data->useCurrentLayer);
    leAngle->setText(QString("%1").arg(RS_Math::rad2deg(data->angle)));
}

void QG_DlgMoveRotate::setData(RS_MoveRotateData* d) {
    data = d;
    init();
}

void QG_DlgMoveRotate::updateData() {
    if (rbMove->isChecked()) {
        data->keepOriginals = false;
    } else if (rbCopy->isChecked()) {
        data->keepOriginals = true;
    }

    data->number = sbNumber->value();
    data->multipleCopies = cbMultiCopy->isChecked();

    data->angle = RS_Math::deg2rad(RS_Math::eval(leAngle->text()));
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}