/****************************************************************************
**
*Options widget for ModifyScale m_action

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
#include "rs_actionmodifyscale.h"
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
        QString factorX = fromDouble(m_action->getFactorX());
        QString factorY = fromDouble(m_action->getFactorY());

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
    m_action = dynamic_cast<RS_ActionModifyScale *>(a);
    assert(m_action != nullptr);
    if (m_action == nullptr)
        return;

    QString factorX;
    QString factorY;

    bool useMultipleCopies = false;
    bool keepOriginals = false;
    bool useCurrentLayer = false;
    bool useCurrentAttributes = false;
    int copiesNumber = 0;

    bool explicitFactor = false;
    bool isotrophic = false;

    if (update){
        useCurrentLayer = m_action->isUseCurrentLayer();
        useCurrentAttributes = m_action->isUseCurrentAttributes();
        keepOriginals = m_action->isKeepOriginals();
        useMultipleCopies = m_action->isUseMultipleCopies();
        copiesNumber = m_action->getCopiesNumber();

        explicitFactor = m_action->isExplicitFactor();
        isotrophic = m_action->isIsotropicScaling();
        factorX = fromDouble(m_action->getFactorX());
        factorY= fromDouble(m_action->getFactorY());
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

    setFactorXToActionAndView(factorX);
    setFactorYToActionAndView(factorY);
    setExplicitFactorToActionAndView(explicitFactor);
    setIsotropicScalingFactorToActionAndView(isotrophic);
}

void LC_ModifyScaleOptions::setUseMultipleCopiesToActionAndView(bool copies) {
    m_action->setUseMultipleCopies(copies);
    ui->cbMultipleCopies->setChecked(copies);
    ui->sbNumberOfCopies->setEnabled(copies);
}

void LC_ModifyScaleOptions::setUseCurrentLayerToActionAndView(bool val) {
    m_action->setUseCurrentLayer(val);
    ui->cbCurrentLayer->setChecked(val);
}

void LC_ModifyScaleOptions::setUseCurrentAttributesToActionAndView(bool val) {
    m_action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_ModifyScaleOptions::setKeepOriginalsToActionAndView(bool val) {
    m_action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_ModifyScaleOptions::setCopiesNumberToActionAndView(int number) {
    if (number < 1){
        number = 1;
    }
    m_action->setCopiesNumber(number);
    ui->sbNumberOfCopies->setValue(number);
}

void LC_ModifyScaleOptions::setIsotropicScalingFactorToActionAndView(bool val) {
    ui->cbIsotrpic->setChecked(val);
    m_action->setIsotropicScaling(val);
    ui->leFactorY->setEnabled(!val && !ui->cbExplicitFactor->isChecked() );
}

void LC_ModifyScaleOptions::setExplicitFactorToActionAndView(bool val) {
    ui->cbExplicitFactor->setChecked(val);
    m_action->setExplicitFactor(val);
    ui->leFactorX->setEnabled(!val);
    ui->leFactorY->setEnabled(!val && !ui->cbIsotrpic->isChecked());
}

void LC_ModifyScaleOptions::setFactorXToActionAndView(QString val) {
    double factor = 1.;
    if (toDouble(val, factor, 0.0, false)) {
        const QString &factorStr = fromDouble(factor);
        ui->leFactorX->setText(factorStr);
        m_action->setFactorX(factor);
    }
}

void LC_ModifyScaleOptions::setFactorYToActionAndView(QString val) {
    double factor = 1.;
    if (toDouble(val, factor, 0.0, false)) {
        const QString &factorStr = fromDouble(factor);
        ui->leFactorY->setText(factorStr);
        m_action->setFactorY(factor);
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
