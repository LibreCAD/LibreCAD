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

#include "lc_rotate2options.h"
#include "ui_lc_rotate2options.h"
#include "rs_math.h"

// todo - potentially, instead of specifying secondary angle it is possible to let the user enter the sum of angles
// todo - and calculate secondary angle based on angle1 and that sum.
// todo - Such approach will simplify ui (as sum defines resulting angle of entity) - yet will break compatibility with
// todo - previous versions. I'm not sure that this is popular command, yet still...
// fixme - think and decide which way of setting secondary angle is more convenient...

LC_Rotate2Options::LC_Rotate2Options()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyRotate2, "Modify", "Rotate2")
    , ui(new Ui::LC_Rotate2Options)
{
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_Rotate2Options::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_Rotate2Options::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_Rotate2Options::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_Rotate2Options::cbUseCurrentLayerClicked);
    connect(ui->cbSameAngleForCopies, &QCheckBox::clicked, this, &LC_Rotate2Options::cbSameAngleForCopiesClicked);
    connect(ui->cbAnglesMirrored, &QCheckBox::clicked, this, &LC_Rotate2Options::cbAnglesMirroredClicked);
    connect(ui->leAngle1, &QLineEdit::editingFinished, this, &LC_Rotate2Options::onAngle1EditingFinished);
    connect(ui->leAngle2, &QLineEdit::editingFinished, this, &LC_Rotate2Options::onAngle2EditingFinished);

    // fixme - remove later if the control will not be reused for some other flag
    ui->cbSameAngleForCopies->hide();
}

LC_Rotate2Options::~LC_Rotate2Options(){
    delete ui;
}

void LC_Rotate2Options::doSaveSettings() {
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
    save("MultipleCopies", ui->cbMultipleCopies->isChecked());
    save("UseCurrentLayer",ui->cbCurrentLayer->isChecked());
    save("UseCurrentAttributes",ui->cbCurrentAttr->isChecked());
    save("Angle1", ui->leAngle1->text());
    save("Angle2", ui->leAngle2->text());
    save("MirrorAngles", ui->cbAnglesMirrored->isChecked());
    save("Copies", ui->sbNumberOfCopies->value());
    save("Angle1", ui->leAngle1->text());
    save("Angle2", ui->leAngle2->text());
    save("SameAngleForCopies", ui->cbSameAngleForCopies->isChecked());
}

void LC_Rotate2Options::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<RS_ActionModifyRotate2 *>(a);
    QString angle1;    
    QString angle2;

    bool useMultipleCopies;
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    int copiesNumber;
    bool sameAngle;
    bool mirrorAngles;

    if (update){
        useCurrentLayer = action->isUseCurrentLayer();
        useCurrentAttributes = action->isUseCurrentAttributes();
        keepOriginals = action->isKeepOriginals();
        useMultipleCopies = action->isUseMultipleCopies();
        copiesNumber = action->getCopiesNumber();
        sameAngle = action->isUseSameAngle2ForCopies();
        mirrorAngles = action->isMirrorAngles();
        angle1 = fromDouble(RS_Math::rad2deg(action->getAngle1()));
        angle2 = fromDouble(RS_Math::rad2deg(action->getAngle2()));
    }
    else{
        useCurrentLayer = loadBool("UseCurrentLayer", false);
        useCurrentAttributes = loadBool("UseCurrentAttributes", false);
        keepOriginals = loadBool("KeepOriginals", false);
        useMultipleCopies = loadBool("MultipleCopies", false);
        copiesNumber = loadInt("Copies", 1);
        sameAngle = loadBool("SameAngleForCopies", false);
        mirrorAngles = loadBool("MirrorAngles", true);
        angle1 = load("Angle1", "30");
        angle2 = load("Angle2", "30");
    }
    setUseMultipleCopiesToActionAndView(useMultipleCopies);
    setCopiesNumberToActionAndView(copiesNumber);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);
    setSameAngleForCopiesToActionAndView(sameAngle);
    setAnglesMirroredToModelAndView(mirrorAngles);
    setAngle1ToActionAndView(angle1);
    setAngle2ToActionAndView(angle2);
}

void LC_Rotate2Options::setUseMultipleCopiesToActionAndView(bool copies) {
    action->setUseMultipleCopies(copies);
    ui->cbMultipleCopies->setChecked(copies);
    ui->sbNumberOfCopies->setEnabled(copies);
    ui->cbSameAngleForCopies->setEnabled(copies);
}

void LC_Rotate2Options::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbCurrentLayer->setChecked(val);
}

void LC_Rotate2Options::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_Rotate2Options::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_Rotate2Options::setCopiesNumberToActionAndView(int number) {
    if (number < 1){
        number = 1;
    }
    action->setCopiesNumber(number);
    ui->sbNumberOfCopies->setValue(number);
}

void LC_Rotate2Options::setAnglesMirroredToModelAndView(bool checked) {
    ui->cbAnglesMirrored->setChecked(checked);
    ui->leAngle2->setEnabled(!checked);
    action->setMirrorAngles(checked);
    setAngle1ToActionAndView(ui->leAngle1->text()); // just force update both edits
}

void LC_Rotate2Options::setSameAngleForCopiesToActionAndView(bool val) {
    action->setUseSameAngle2ForCopies(val);
    ui->cbSameAngleForCopies->setChecked(val);
}

void LC_Rotate2Options::setAngle1ToActionAndView(QString val) {
    double angle;
    if (toDoubleAngle(val, angle, 0.0, false)){
        ui->leAngle1->setText(fromDouble(angle));
        action->setAngle1(RS_Math::deg2rad(angle));
        bool anglesMirrored = ui->cbAnglesMirrored->isChecked();
        if (anglesMirrored){
            ui->leAngle2->setText(fromDouble(-angle));
            action->setAngle2(RS_Math::deg2rad(-angle));
        }
    }
}

void LC_Rotate2Options::setAngle2ToActionAndView(QString val) {
    double angle;
    if (toDoubleAngle(val, angle, 0.0, false)) {
        const QString &angleStr = fromDouble(angle);
        ui->leAngle2->setText(angleStr);
        action->setAngle2(RS_Math::deg2rad(angle));
    }        
}

void LC_Rotate2Options::languageChange() {
    ui->retranslateUi(this);
}

void LC_Rotate2Options::onAngle1EditingFinished(){
    const QString &expr = ui->leAngle1->text();
    setAngle1ToActionAndView(expr);
}

void LC_Rotate2Options::onAngle2EditingFinished(){
    const QString &expr = ui->leAngle2->text();
    setAngle2ToActionAndView(expr);
}

void LC_Rotate2Options::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_Rotate2Options::cbMultipleCopiesClicked(bool val) {
    setUseMultipleCopiesToActionAndView(val);
}

void LC_Rotate2Options::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_Rotate2Options::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void LC_Rotate2Options::cbSameAngleForCopiesClicked(bool val) {
    setSameAngleForCopiesToActionAndView(val);
}

void LC_Rotate2Options::cbAnglesMirroredClicked(bool checked) {
    setAnglesMirroredToModelAndView(checked);
}

void LC_Rotate2Options::on_sbNumberOfCopies_valueChanged(int number) {
    setCopiesNumberToActionAndView(number);
}
