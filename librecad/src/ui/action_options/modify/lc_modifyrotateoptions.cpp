/****************************************************************************
**
* Options widget for ModifyRotate action

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
#include "lc_modifyrotateoptions.h"
#include "rs_actionmodifyrotate.h"
#include "ui_lc_modifyrotateoptions.h"

LC_ModifyRotateOptions::LC_ModifyRotateOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyRotate, "Modify", "Rotate")
    , ui(new Ui::LC_ModifyRotateOptions){
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::cbUseCurrentLayerClicked);

    connect(ui->cbFreeAngle, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::cbFreeAngleClicked);
    connect(ui->cbFreeRefAngle, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::cbFreeRefAngleClicked);
    connect(ui->cbTwoRotations, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::onTwoRotationsClicked);
    connect(ui->cbAbsoluteRefAngle, &QCheckBox::clicked, this, &LC_ModifyRotateOptions::onAbsoluteRefAngleClicked);

    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_ModifyRotateOptions::onCopiesNumberValueChanged);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_ModifyRotateOptions::onAngleEditingFinished);
    connect(ui->leAngle2, &QLineEdit::editingFinished, this,&LC_ModifyRotateOptions::onRefPointAngleEditingFinished);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickAngleSetup("angle2", ui->tbPickAngle2, ui->leAngle2);
}

LC_ModifyRotateOptions::~LC_ModifyRotateOptions(){
    delete ui;
}

void LC_ModifyRotateOptions::doSaveSettings() {
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
    save("MultipleCopies", ui->cbMultipleCopies->isChecked());
    save("Copies", ui->sbNumberOfCopies->value());
    save("UseCurrentLayer",ui->cbCurrentLayer->isChecked());
    save("UseCurrentAttributes",ui->cbCurrentAttr->isChecked());
    save("Angle", ui->leAngle->text());
    save("AngleIsFree", ui->cbFreeAngle->isChecked());
    save("TwoRotations",     ui->cbTwoRotations->isChecked());
    save("AngleRef", ui->leAngle2->text());
    save("AngleRefIsFree", ui->cbFreeRefAngle->isChecked());
    save("AngleRefIsAbsolute", ui->cbAbsoluteRefAngle->isChecked());
}

void LC_ModifyRotateOptions::updateUI(int mode) {
    switch (mode){
        case UPDATE_ANGLE: {  // update on SetTargetPoint
            QString angle = fromDouble(m_action->getCurrentAngleDegrees());

            ui->leAngle->blockSignals(true);
            ui->leAngle->setText(angle);
            ui->leAngle->blockSignals(false);

            ui->leAngle->update();
            break;
        }
        case DISABLE_SECOND_ROTATION:{
            allowSecondRotationUI(false);
            break;
        }
        case ENABLE_SECOND_ROTATION:{
            allowSecondRotationUI(true);
            break;
        }
        case UPDATE_ANGLE2: {  // update on SetTargetPoint
            QString angle2 = fromDouble(m_action->getCurrentAngle2Degrees());

            ui->leAngle2->blockSignals(true);
            ui->leAngle2->setText(angle2);
            ui->leAngle2->blockSignals(false);

            ui->leAngle2->update();
            break;
        }
        default:
            break;
    }
}

void LC_ModifyRotateOptions::allowSecondRotationUI(bool enable) {
    bool enableSecondAngle = enable && !ui->cbFreeRefAngle->isChecked();
    ui->leAngle2->setEnabled(enableSecondAngle);
    ui->tbPickAngle2->setEnabled(enableSecondAngle);
    ui->cbTwoRotations->setEnabled(enable);
    ui->cbFreeRefAngle->setEnabled(enable);
    ui->cbAbsoluteRefAngle->setEnabled(enable);

}

void LC_ModifyRotateOptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<RS_ActionModifyRotate *>(a);
    QString angle;
    QString angle2;

    bool useMultipleCopies;
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    int copiesNumber;

    bool freeAngle;
    bool freeRefAngle;
    bool absoluteRefAngle;
    bool twoRotations;

    if (update){
        useCurrentLayer = m_action->isUseCurrentLayer();
        useCurrentAttributes = m_action->isUseCurrentAttributes();
        keepOriginals = m_action->isKeepOriginals();
        useMultipleCopies = m_action->isUseMultipleCopies();
        copiesNumber = m_action->getCopiesNumber();

        twoRotations = m_action->isRotateAlsoAroundReferencePoint();
        freeAngle = m_action->isFreeAngle();
        freeRefAngle = m_action->isFreeRefPointAngle();
        absoluteRefAngle = m_action->isRefPointAngleAbsolute();
        angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
        angle2 = fromDouble(RS_Math::rad2deg(m_action->getRefPointAngle()));
    }
    else{
        useCurrentLayer = loadBool("UseCurrentLayer", true);
        useCurrentAttributes = loadBool("UseCurrentAttributes", true);
        keepOriginals = loadBool("KeepOriginals", false);
        useMultipleCopies = loadBool("MultipleCopies", false);
        copiesNumber = loadInt("Copies", 1);

        twoRotations = loadBool("TwoRotations", false);
        freeAngle = loadBool("AngleIsFree", true);
        freeRefAngle = loadBool("AngleRefIsFree", false);
        absoluteRefAngle = loadBool("AngleRefIsAbsolute", false);
        angle = load("Angle", "0.0");
        angle2= load("AngleRef", "0.0");
    }
    setUseMultipleCopiesToActionAndView(useMultipleCopies);
    setCopiesNumberToActionAndView(copiesNumber);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);

    setAngleToActionAndView(angle);
    setFreeAngleToActionAndView(freeAngle);

    setFreeRefAngleToActionAndView(freeRefAngle);
    setRefPointAngleToActionAndView(angle2);
    setAbsoluteRefAngleToActionAndView(absoluteRefAngle);
    setTwoRotationsToActionAndView(twoRotations);
}

void LC_ModifyRotateOptions::setUseMultipleCopiesToActionAndView(bool copies) {
    m_action->setUseMultipleCopies(copies);
    ui->cbMultipleCopies->setChecked(copies);
    ui->sbNumberOfCopies->setEnabled(copies);
}

void LC_ModifyRotateOptions::setUseCurrentLayerToActionAndView(bool val) {
    m_action->setUseCurrentLayer(val);
    ui->cbCurrentLayer->setChecked(val);
}

void LC_ModifyRotateOptions::setUseCurrentAttributesToActionAndView(bool val) {
    m_action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_ModifyRotateOptions::setKeepOriginalsToActionAndView(bool val) {
    m_action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_ModifyRotateOptions::setCopiesNumberToActionAndView(int number) {
    if (number < 1){
        number = 1;
    }
    m_action->setCopiesNumber(number);
    ui->sbNumberOfCopies->setValue(number);
}

void LC_ModifyRotateOptions::setTwoRotationsToActionAndView(bool val) {
    allowSecondRotationUI(val);
    ui->cbTwoRotations->setEnabled(true);
    ui->cbTwoRotations->setChecked(val);
    m_action->setRotateAlsoAroundReferencePoint(val);
}

void LC_ModifyRotateOptions::setFreeAngleToActionAndView(bool val) {
    ui->cbFreeAngle->setChecked(val);
    m_action->setFreeAngle(val);
    if (val){
        ui->leAngle->setEnabled(false);
        ui->tbPickAngle->setEnabled(false);
    }
    else{
        ui->leAngle->setEnabled(true);
        ui->tbPickAngle->setEnabled(true);
    }
}

void LC_ModifyRotateOptions::setAbsoluteRefAngleToActionAndView(bool checked){
    ui->cbAbsoluteRefAngle->setChecked(checked);
    m_action->setRefPointAngleAbsolute(checked);
}

void LC_ModifyRotateOptions::setFreeRefAngleToActionAndView(bool checked) {
    ui->cbFreeRefAngle->setChecked(checked);
    if (ui->cbTwoRotations->isChecked()) {
        ui->leAngle2->setEnabled(!checked);
        ui->tbPickAngle2->setEnabled(!checked);
    }
    m_action->setFreeRefPointAngle(checked);
}

void LC_ModifyRotateOptions::setAngleToActionAndView(QString val) {
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        const QString &factorStr = fromDouble(angle);
        ui->leAngle->setText(factorStr);
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
}

void LC_ModifyRotateOptions::setRefPointAngleToActionAndView(QString val) {
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        const QString &factorStr = fromDouble(angle);
        ui->leAngle2->setText(factorStr);
        m_action->setRefPointAngle(RS_Math::deg2rad(angle));
    }
}

void LC_ModifyRotateOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_ModifyRotateOptions::cbMultipleCopiesClicked(bool val) {
    setUseMultipleCopiesToActionAndView(val);
}

void LC_ModifyRotateOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_ModifyRotateOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void LC_ModifyRotateOptions::cbFreeAngleClicked(bool val) {
    setFreeAngleToActionAndView(val);
}

void LC_ModifyRotateOptions::cbFreeRefAngleClicked(bool val) {
    setFreeRefAngleToActionAndView(val);
}

void LC_ModifyRotateOptions::onAbsoluteRefAngleClicked(bool val){
    setAbsoluteRefAngleToActionAndView(val);
}

void LC_ModifyRotateOptions::onTwoRotationsClicked(bool val) {
    setTwoRotationsToActionAndView(val);
}

void LC_ModifyRotateOptions::onAngleEditingFinished() {
    setAngleToActionAndView(ui->leAngle->text());
}

void LC_ModifyRotateOptions::onRefPointAngleEditingFinished() {
    setRefPointAngleToActionAndView(ui->leAngle2->text());
}

void LC_ModifyRotateOptions::onCopiesNumberValueChanged(int value) {
    setCopiesNumberToActionAndView(value);
}

void LC_ModifyRotateOptions::languageChange() {
    ui->retranslateUi(this);
}
