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

#include "lc_moveoptions.h"
#include "ui_lc_moveoptions.h"

LC_MoveOptions::LC_MoveOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyMove, "Modify", "Move")
    , ui(new Ui::LC_MoveOptions){
    ui->setupUi(this);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_MoveOptions::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_MoveOptions::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_MoveOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_MoveOptions::cbUseCurrentLayerClicked);}

LC_MoveOptions::~LC_MoveOptions(){
    delete ui;
}

void LC_MoveOptions::doSaveSettings() {
    save("UseCurrentLayer", ui->cbCurrentLayer->isChecked());
    save("UseCurrentAttributes", ui->cbCurrentAttr->isChecked());
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
    save("MultipleCopies", ui->cbMultipleCopies->isChecked());
    save("Copies", ui->sbNumberOfCopies->value());
}

void LC_MoveOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyMove *>(a);
    bool useMultipleCopies;
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    int copiesNumber;
    if (update){
        useCurrentLayer = action->isUseCurrentLayer();
        useCurrentAttributes  = action->isUseCurrentAttributes();
        copiesNumber = action->getCopiesNumber();
        keepOriginals = action->isKeepOriginals();
        useMultipleCopies = action->isUseMultipleCopies();
    }
    else{
        useCurrentLayer = loadBool("UseCurrentLayer", false);
        useCurrentAttributes = loadBool("UseCurrentAttributes", false);
        keepOriginals = loadBool("KeepOriginals", false);
        useMultipleCopies = loadBool("MultipleCopies", false);
        copiesNumber = loadInt("Copies", 1);
    }
    setUseMultipleCopiesToActionAndView(useMultipleCopies);
    setCopiesNumberToActionAndView(copiesNumber);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);
}

void LC_MoveOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_MoveOptions::setCopiesNumberToActionAndView(int number) {
    if (number < 1){
        number = 1;
    }
    action->setCopiesNumber(number);
    ui->sbNumberOfCopies->setValue(number);
}

void LC_MoveOptions::setUseMultipleCopiesToActionAndView(bool copies) {
   action->setUseMultipleCopies(copies);
   ui->cbMultipleCopies->setChecked(copies);
   ui->sbNumberOfCopies->setEnabled(copies);
}

void LC_MoveOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbCurrentLayer->setChecked(val);
}

void LC_MoveOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_MoveOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_MoveOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_MoveOptions::cbMultipleCopiesClicked(bool val) {
    setUseMultipleCopiesToActionAndView(val);
}

void LC_MoveOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_MoveOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void LC_MoveOptions::on_sbNumberOfCopies_valueChanged(int number) {
    setCopiesNumberToActionAndView(number);
}
