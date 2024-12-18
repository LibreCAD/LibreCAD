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

#include "lc_modifyalignrefoptions.h"
#include "ui_lc_modifyalignrefoptions.h"

LC_ModifyAlignRefOptions::LC_ModifyAlignRefOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionModifyAlignRef, "Modify", "AlignRef")
    , ui(new Ui::LC_ModifyAlignRefOptions){
    ui->setupUi(this);

    connect(ui->cbScale,  &QCheckBox::toggled, this, &LC_ModifyAlignRefOptions::onScaleClicked);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_ModifyAlignRefOptions::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_ModifyAlignRefOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::clicked, this, &LC_ModifyAlignRefOptions::cbUseCurrentLayerClicked);
}

LC_ModifyAlignRefOptions::~LC_ModifyAlignRefOptions(){
    delete ui;
}

void LC_ModifyAlignRefOptions::doSaveSettings() {
    save("Scale", ui->cbScale->isChecked());
    save("UseCurrentLayer", ui->cbLayer->isChecked());
    save("UseCurrentAttributes", ui->cbCurrentAttr->isChecked());
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
}

void LC_ModifyAlignRefOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<LC_ActionModifyAlignRef *>(a);
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    bool scale;
    if (update){
        scale = action->isScale();
        useCurrentLayer = action->isUseCurrentLayer();
        useCurrentAttributes  = action->isUseCurrentAttributes();
        keepOriginals = action->isKeepOriginals();
    }
    else{
        useCurrentLayer = loadBool("UseCurrentLayer", false);
        useCurrentAttributes = loadBool("UseCurrentAttributes", false);
        keepOriginals = loadBool("KeepOriginals", false);
        scale = loadBool("Scale", true);
    }

    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);

    setScaleToActionAndView(scale);
}

void LC_ModifyAlignRefOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_ModifyAlignRefOptions::onScaleClicked([[maybe_unused]]bool clicked) {
    setScaleToActionAndView(ui->cbScale->isChecked());
}

void LC_ModifyAlignRefOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_ModifyAlignRefOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_ModifyAlignRefOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void LC_ModifyAlignRefOptions::setScaleToActionAndView(bool val) {
    action->setScale(val);
    ui->cbScale->setChecked(val);
}

void LC_ModifyAlignRefOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbLayer->setChecked(val);
}

void LC_ModifyAlignRefOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_ModifyAlignRefOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}
