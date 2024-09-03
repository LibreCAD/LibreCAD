/****************************************************************************
**
* Options widget for pen transform action

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
#include "lc_pastetransformoptions.h"
#include "ui_lc_pastetransformoptions.h"
#include "rs_math.h"

LC_PasteTransformOptions::LC_PasteTransformOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionEditPasteTransform, "Edit", "PasteTransform")
    , ui(new Ui::LC_PasteTransformOptions){
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_PasteTransformOptions::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &LC_PasteTransformOptions::onFactorEditingFinished);
    connect(ui->leArraySpacingX, &QLineEdit::editingFinished, this, &LC_PasteTransformOptions::onArraySpacingXEditingFinished);
    connect(ui->leArraySpacingY, &QLineEdit::editingFinished, this, &LC_PasteTransformOptions::onArraySpacingYEditingFinished);
    connect(ui->leArrayAngle, &QLineEdit::editingFinished, this, &LC_PasteTransformOptions::onArrayAngleEditingFinished);
    connect(ui->cbArray, &QCheckBox::clicked, this, &LC_PasteTransformOptions::onArrayClicked);
    connect(ui->sbArrayX, &QSpinBox::valueChanged, this, &LC_PasteTransformOptions::onArrayXCountChanged);
    connect(ui->sbArrayY, &QSpinBox::valueChanged, this, &LC_PasteTransformOptions::onArrayYCountChanged);
    connect(ui->cbSameAngles, &QCheckBox::clicked, this, &LC_PasteTransformOptions::cbSameAnglesClicked);
}

LC_PasteTransformOptions::~LC_PasteTransformOptions(){delete ui;}


void LC_PasteTransformOptions::doSaveSettings() {
    save("Angle", ui->leAngle->text());
    save("ScaleFactor", ui->leFactor->text());
    save("IsArray", ui->cbArray->isChecked());
    save("ArrayXCount", ui->sbArrayX->value());
    save("ArrayYCount", ui->sbArrayY->value());
    save("ArrayXSpacing", ui->leArraySpacingX->text());
    save("ArrayYSpacing", ui->leArraySpacingY->text());
    save("ArrayAngle", ui->leArrayAngle->text());
    save("SameAngles", ui->cbSameAngles->isChecked());
}

void LC_PasteTransformOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<LC_ActionEditPasteTransform *>(a);
    QString angle;
    QString factor;
    bool isArray;
    int arrayXCount;
    int arrayYCount;
    QString arrayXSpacing;
    QString arrayYSpacing;
    bool sameAngles;
    QString arrayAngle;
    if (update){
        angle = fromDouble(RS_Math::rad2deg(action->getAngle()));
        factor = fromDouble(action->getFactor());
        isArray = action->isArrayCreated();
        arrayXCount = action -> getArrayXCount();
        arrayYCount = action -> getArrayYCount();
        arrayXSpacing  = fromDouble(action->getArraySpacingX());
        arrayYSpacing  = fromDouble(action->getArraySpacingY());
        sameAngles = action->isSameAngles();
        arrayAngle = fromDouble(RS_Math::rad2deg(action->getArrayAngle()));
    }
    else{
        angle = load("Angle", "0.0");
        factor = load("ScaleFactor", "1.0");
        isArray = loadBool("IsArray", false);
        arrayXCount = loadInt("ArrayXCount", 1);
        arrayYCount = loadInt("ArrayYCount", 1);
        arrayXSpacing  = load("ArrayXSpacing", "10.0");
        arrayYSpacing  = load("ArrayYSpacing", "10.0");
        arrayAngle = load("ArrayAngle", "0.0");
        sameAngles = loadBool("SameAngles", false);
    }

    setAngleToActionAndView(angle);
    setFactorToActionAndView(factor);
    setIsArrayToActionAndView(isArray);
    setArrayXCountToActionAndView(arrayXCount);
    setArrayYCountToActionAndView(arrayYCount);
    setArrayXSpacingToActionAndView(arrayXSpacing);
    setArrayYSpacingToActionAndView(arrayYSpacing);
    setArrayAngleToActionAndView(arrayAngle);
    setSameAnglesToActionAndView(sameAngles);
}

void LC_PasteTransformOptions::setAngleToActionAndView(QString val) {
    double angle;
    if (toDoubleAngle(val, angle, 0.0, false)){
        action->setAngle(RS_Math::deg2rad(angle));
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_PasteTransformOptions::setFactorToActionAndView(QString val) {
    double y;
    if (toDouble(val, y, 1.0, true)){
        action->setFactor(y);
        ui->leFactor->setText(fromDouble(y));
    }
}

void LC_PasteTransformOptions::setIsArrayToActionAndView(bool val) {
    action->setArrayCreated(val);
    ui->cbArray->setChecked(val);
    ui->sbArrayX->setEnabled(val);
    ui->sbArrayY->setEnabled(val);
    ui->leArraySpacingX->setEnabled(val);
    ui->leArraySpacingY->setEnabled(val);
    ui->leArrayAngle->setEnabled(val);
    ui->cbSameAngles->setEnabled(val);
    if (val){
        ui->leAngle->setEnabled(!ui->cbSameAngles->isChecked());
    }
    else{
        ui->leAngle->setEnabled(true);
    }
}

void LC_PasteTransformOptions::setSameAnglesToActionAndView(bool val) {
    ui->cbSameAngles ->setChecked(val);    
    if (val) {
        ui->leAngle->setText(ui->leArrayAngle->text());
        bool enable = !ui->cbArray->isChecked();
        ui->leAngle->setEnabled(enable);
    }
    else{
        ui->leAngle->setEnabled(true);
    }
    action->setSameAngles(val);
}

void LC_PasteTransformOptions::setArrayXCountToActionAndView(int count) {
    action->setArrayXCount(count);
    ui->sbArrayX->setValue(count);
}

void LC_PasteTransformOptions::setArrayYCountToActionAndView(int count) {
    action->setArrayYCount(count);
    ui->sbArrayY->setValue(count);
}

void LC_PasteTransformOptions::setArrayXSpacingToActionAndView(QString val) {
    double y;
    if (toDouble(val, y, 1.0, true)){
        action->setArraySpacingX(y);
        ui->leArraySpacingX->setText(fromDouble(y));
    }
}

void LC_PasteTransformOptions::setArrayYSpacingToActionAndView(QString val) {
    double y;
    if (toDouble(val, y, 1.0, true)){
        action->setArraySpacingY(y);
        ui->leArraySpacingY->setText(fromDouble(y));
    }
}

void LC_PasteTransformOptions::setArrayAngleToActionAndView(QString val) {
    double angle;
    if (toDoubleAngle(val, angle, 0.0, false)){
        action->setArrayAngle(RS_Math::deg2rad(angle));
        const QString &angleStr = fromDouble(angle);
        ui->leArrayAngle->setText(angleStr);
        if (ui->cbSameAngles->isChecked()){
            ui->leAngle->setText(angleStr);
        }
    }
}

void LC_PasteTransformOptions::onArrayXCountChanged(int value){
    setArrayXCountToActionAndView(value);
}

void LC_PasteTransformOptions::onArrayYCountChanged(int value){
    setArrayYCountToActionAndView(value);
}

void LC_PasteTransformOptions::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void LC_PasteTransformOptions::onFactorEditingFinished(){
    setFactorToActionAndView(ui->leFactor->text());
}

void LC_PasteTransformOptions::onArraySpacingXEditingFinished(){
    setArrayXSpacingToActionAndView(ui->leArraySpacingX->text());
}

void LC_PasteTransformOptions::onArraySpacingYEditingFinished(){
    setArrayYSpacingToActionAndView(ui->leArraySpacingY->text());
}
void LC_PasteTransformOptions::onArrayAngleEditingFinished(){
    setArrayAngleToActionAndView(ui->leArrayAngle->text());
}

void LC_PasteTransformOptions::onArrayClicked(bool clicked){
    setIsArrayToActionAndView(clicked);
}

void LC_PasteTransformOptions::cbSameAnglesClicked(bool value) {
    setSameAnglesToActionAndView(value);
}

void LC_PasteTransformOptions::languageChange() {
    ui->retranslateUi(this);
}
