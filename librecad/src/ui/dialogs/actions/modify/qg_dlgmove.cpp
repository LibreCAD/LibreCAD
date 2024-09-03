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
#include "rs_math.h"
#include "qg_dlgmove.h"

#include "rs_settings.h"
#include "rs_modification.h"

/*
 *  Constructs a QG_DlgMove as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgMove::QG_DlgMove(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl){
    setModal(modal);
    setupUi(this);
    connect(cbMultipleCopies, &QCheckBox::clicked, this, &QG_DlgMove::cbMultipleCopiesClicked);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgMove::~QG_DlgMove(){
    destroy();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgMove::languageChange(){
    retranslateUi(this);
}

void QG_DlgMove::cbMultipleCopiesClicked(bool val) {
   sbNumber->setEnabled(val);
}

void QG_DlgMove::init() {
    rbCopy->setChecked(data->keepOriginals);
    rbMove->setChecked(!data->keepOriginals);
    cbMultipleCopies -> setChecked(data->multipleCopies);

    sbNumber->setValue(data->number);
    cbCurrentAttributes->setChecked(data->useCurrentAttributes);
    cbCurrentLayer->setChecked(data->useCurrentLayer);
    sbNumber->setEnabled(data->multipleCopies);
}

void QG_DlgMove::setData(RS_MoveData* d) {
    data = d;
    init();
}

void QG_DlgMove::updateData() {
    if (rbMove->isChecked()) {
        data->keepOriginals = false;
    } else if (rbCopy->isChecked()) {
        data->keepOriginals = true;
    }
    data->number = sbNumber->value();
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
    data->multipleCopies = cbMultipleCopies->isChecked();
}

