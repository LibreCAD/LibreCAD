/****************************************************************************
**
*Options widget for ModifyScale action

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
**********************************************************************/

#include "lc_modifyscaleoptions.h"
#include "ui_lc_modifyscaleoptions.h"

LC_ModifyScaleOptions::LC_ModifyScaleOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyScale, "Modify", "Scale")
    , ui(new Ui::LC_ModifyScaleOptions){
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_ModifyScaleOptions::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_ModifyScaleOptions::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_ModifyScaleOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_ModifyScaleOptions::cbUseCurrentLayerClicked);

    connect(ui->cbExplicitFactor, &QCheckBox::clicked, this, &LC_ModifyScaleOptions::cbExplicitFactorClicked);
    connect(ui->cbIsotrpic, &QCheckBox::clicked, this, &LC_ModifyScaleOptions::cbIsotropicClicked);
    connect(ui->leFactorX, &QLineEdit::editingFinished, this, &LC_ModifyScaleOptions::onFactorXEditingFinished);
    connect(ui->leFactorY, &QLineEdit::editingFinished, this, &LC_ModifyScaleOptions::onFactorYEditingFinished);
    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_ModifyScaleOptions::onCopiesNumberValueChanged);
}

LC_ModifyScaleOptions::~LC_ModifyScaleOptions(){
    delete ui;
}

void LC_ModifyScaleOptions::doSaveSettings() {
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
    save("MultipleCopies", ui->cbMultipleCopies->isChecked());
    save("Copies", ui->sbNumberOfCopies->value());
    save("UseCurrentLayer",ui->cbCurrentLayer->isChecked());
    save("UseCurrentAttributes",ui->cbCurrentAttr->isChecked());
    save("FactorX", ui->leFactorX->text());
    save("FactorY", ui->leFactorY->text());
    save("Isotropic",     ui->cbIsotrpic->isChecked());
    save("ExplicitFactor", ui->cbExplicitFactor->isChecked());
}

void LC_ModifyScaleOptions::updateUI(int mode) {
    if (mode == 0) { // update on SetTargetPoint
        QString factorX = fromDouble(action->getFactorX());
        QString factorY = fromDouble(action->getFactorY());

        ui->leFactorX->blockSignals(true);
        ui->leFactorY->blockSignals(true);

        ui->leFactorX->setText(factorX);
        if (ui->cbIsotrpic) {
            ui->leFactorY->setText(factorX);
        } else {
            ui->leFactorY->setText(factorY);
        }

        ui->leFactorX->blockSignals(false);
        ui->leFactorY->blockSignals(false);
    }
    else{
        // do nothing
    }
}

void LC_ModifyScaleOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyScale *>(a);
    QString factorX;
    QString factorY;

    bool useMultipleCopies;
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    int copiesNumber;

    bool explicitFactor;
    bool isotrophic;

    if (update){
        useCurrentLayer = action->isUseCurrentLayer();
        useCurrentAttributes = action->isUseCurrentAttributes();
        keepOriginals = action->isKeepOriginals();
        useMultipleCopies = action->isUseMultipleCopies();
        copiesNumber = action->getCopiesNumber();

        explicitFactor = action->isExplicitFactor();
        isotrophic = action->isIsotropicScaling();
        factorX = fromDouble(action->getFactorX());
        factorY= fromDouble(action->getFactorY());
    }
    else{
        useCurrentLayer = loadBool("UseCurrentLayer", false);
        useCurrentAttributes = loadBool("UseCurrentAttributes", false);
        keepOriginals = loadBool("KeepOriginals", false);
        useMultipleCopies = loadBool("MultipleCopies", false);
        copiesNumber = loadInt("Copies", 1);

        explicitFactor = loadBool("ExplicitFactor", false);
        isotrophic = loadBool("Isotropic", true);
        factorX = load("FactorX", "1.1");
        factorY= load("FactorY", "1.1");
    }
    setUseMultipleCopiesToActionAndView(useMultipleCopies);
    setCopiesNumberToActionAndView(copiesNumber);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);

    setExplicitFactorToActionAndView(explicitFactor);
    setIsotropicScalingFactorToActionAndView(isotrophic);
    setFactorXToActionAndView(factorX);
    setFactorYToActionAndView(factorY);
}

void LC_ModifyScaleOptions::setUseMultipleCopiesToActionAndView(bool copies) {
    action->setUseMultipleCopies(copies);
    ui->cbMultipleCopies->setChecked(copies);
    ui->sbNumberOfCopies->setEnabled(copies);
}

void LC_ModifyScaleOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbCurrentLayer->setChecked(val);
}

void LC_ModifyScaleOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_ModifyScaleOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_ModifyScaleOptions::setCopiesNumberToActionAndView(int number) {
    if (number < 1){
        number = 1;
    }
    action->setCopiesNumber(number);
    ui->sbNumberOfCopies->setValue(number);
}

void LC_ModifyScaleOptions::setIsotropicScalingFactorToActionAndView(bool val) {
    ui->cbIsotrpic->setChecked(val);
    action->setIsotropicScaling(val);
    ui->leFactorY->setEnabled(!val);
}

void LC_ModifyScaleOptions::setExplicitFactorToActionAndView(bool val) {
    ui->cbExplicitFactor->setChecked(val);
    action->setExplicitFactor(val);
    if (val){
        ui->leFactorX->setEnabled(false);
        ui->leFactorY->setEnabled(false);
    }
    else{
        ui->leFactorX->setEnabled(true);
        ui->leFactorY->setEnabled(!ui->cbIsotrpic->isChecked());
    }
}

void LC_ModifyScaleOptions::setFactorXToActionAndView(QString val) {
    double factor;
    if (toDouble(val, factor, 0.0, false)) {
        const QString &factorStr = fromDouble(factor);
        ui->leFactorX->setText(factorStr);
        action->setFactorX(factor);
    }
}

void LC_ModifyScaleOptions::setFactorYToActionAndView(QString val) {
    double factor;
    if (toDouble(val, factor, 0.0, false)) {
        const QString &factorStr = fromDouble(factor);
        ui->leFactorY->setText(factorStr);
        action->setFactorY(factor);
    }
}

void LC_ModifyScaleOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_ModifyScaleOptions::cbMultipleCopiesClicked(bool val) {
    setUseMultipleCopiesToActionAndView(val);
}

void LC_ModifyScaleOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_ModifyScaleOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void LC_ModifyScaleOptions::cbExplicitFactorClicked(bool val) {
    setExplicitFactorToActionAndView(val);
}

void LC_ModifyScaleOptions::cbIsotropicClicked(bool val) {
    setIsotropicScalingFactorToActionAndView(val);
}

void LC_ModifyScaleOptions::onFactorXEditingFinished() {
    setFactorXToActionAndView(ui->leFactorX->text());
}

void LC_ModifyScaleOptions::onFactorYEditingFinished() {
    setFactorYToActionAndView(ui->leFactorY->text());
}

void LC_ModifyScaleOptions::onCopiesNumberValueChanged(int value) {
    setCopiesNumberToActionAndView(value);
}

void LC_ModifyScaleOptions::languageChange() {
    ui->retranslateUi(this);
}
