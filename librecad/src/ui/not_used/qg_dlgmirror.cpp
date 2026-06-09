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
#include "qg_dlgmirror.h"

#include "rs_settings.h"
#include "rs_modification.h"

/*
 *  Constructs a QG_DlgMirror as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgMirror::QG_DlgMirror(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl){
    setModal(modal);
    setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgMirror::~QG_DlgMirror(){
    destroy();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgMirror::languageChange(){
    retranslateUi(this);
}

void QG_DlgMirror::init() const {
    rbCopy->setChecked(data->keepOriginals);
    rbMove->setChecked(!data->keepOriginals);

    cbCurrentAttributes->setChecked(data->useCurrentAttributes);
    cbCurrentLayer->setChecked(data->useCurrentLayer);
}

void QG_DlgMirror::setData(RS_MirrorData* d) {
    data = d;
    init();
}

void QG_DlgMirror::updateData() const {
    if (rbMove->isChecked()) {
        data->keepOriginals = false;
    } else if (rbCopy->isChecked()) {
        data->keepOriginals = true;
    }
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}
