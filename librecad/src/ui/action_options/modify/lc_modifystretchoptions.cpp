/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_modifystretchoptions.h"
#include "ui_lc_modifystretchoptions.h"

LC_ModifyStretchOptions::LC_ModifyStretchOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyStretch, "Modify", "Stretch")
    , ui(new Ui::LC_ModifyStretchOptions){
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_ModifyStretchOptions::onKeepOriginalsClicked);
}

LC_ModifyStretchOptions::~LC_ModifyStretchOptions(){
    delete ui;
    action = nullptr;
}

void LC_ModifyStretchOptions::doSaveSettings() {
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
}

void LC_ModifyStretchOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyStretch *>(a);
    bool keepOriginals;
    if (update){
        keepOriginals = !action->isRemoveOriginals();
    }
    else{
        keepOriginals = loadBool("KeepOriginals", false);
    }
    setKeepOriginalsToActionAndView(keepOriginals);
}

void LC_ModifyStretchOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_ModifyStretchOptions::onKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_ModifyStretchOptions::setKeepOriginalsToActionAndView(bool val) {
    ui->cbKeepOriginals->setChecked(val);
    action->setRemoveOriginals(!val);
}
