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

#include "lc_modifymirroroptions.h"
#include "ui_lc_modifymirroroptions.h"

LC_ModifyMirrorOptions::LC_ModifyMirrorOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyMirror,"Modify", "Mirror")
    , ui(new Ui::LC_ModifyMirrorOptions), action(nullptr){
    ui->setupUi(this);

    connect(ui->cbMirrorToLine, SIGNAL(clicked(bool)), this, SLOT(onMirrorToLineClicked(bool)));
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_ModifyMirrorOptions::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_ModifyMirrorOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::clicked, this, &LC_ModifyMirrorOptions::cbUseCurrentLayerClicked);
}

LC_ModifyMirrorOptions::~LC_ModifyMirrorOptions(){
    delete ui;
    action = nullptr;
}

void LC_ModifyMirrorOptions::doSaveSettings() {
    save("ToLine", ui->cbMirrorToLine->isChecked());
    save("UseCurrentLayer", ui->cbLayer->isChecked());
    save("UseCurrentAttributes", ui->cbCurrentAttr->isChecked());
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
}

void LC_ModifyMirrorOptions::onMirrorToLineClicked(bool clicked){
    setMirrorToLineLineToActionAndView(clicked);
}

void LC_ModifyMirrorOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyMirror *>(a);
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    bool useLine;
    if (update){
        useLine = action->isMirrorToExistingLine();
        useCurrentLayer = action->isUseCurrentLayer();
        useCurrentAttributes  = action->isUseCurrentAttributes();
        keepOriginals = action->isKeepOriginals();
    }
    else{
        useCurrentLayer = loadBool("UseCurrentLayer", false);
        useCurrentAttributes = loadBool("UseCurrentAttributes", false);
        keepOriginals = loadBool("KeepOriginals", false);
        useLine = loadBool("ToLine", false);
    }

    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);

    setMirrorToLineLineToActionAndView(useLine);
}

void LC_ModifyMirrorOptions::setMirrorToLineLineToActionAndView(bool value){
    action->setMirrorToExistingLine(value);
    ui->cbMirrorToLine->setChecked(value);
}

void LC_ModifyMirrorOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_ModifyMirrorOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbLayer->setChecked(val);
}

void LC_ModifyMirrorOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_ModifyMirrorOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_ModifyMirrorOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_ModifyMirrorOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_ModifyMirrorOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}
