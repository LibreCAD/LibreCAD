/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/
#include "qg_moverotateoptions.h"

#include "rs_actionmodifymoverotate.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_moverotateoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_MoveRotateOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_MoveRotateOptions::QG_MoveRotateOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyMoveRotate, "Modify","MoveRotate")
	, ui(new Ui::Ui_MoveRotateOptions{}){
    ui->setupUi(this);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &QG_MoveRotateOptions::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &QG_MoveRotateOptions::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &QG_MoveRotateOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &QG_MoveRotateOptions::cbUseCurrentLayerClicked);
    connect(ui->cbSameAngleForCopies, &QCheckBox::clicked, this, &QG_MoveRotateOptions::cbSameAngleForCopiesClicked);
    connect(ui->cbFreeAngle, &QCheckBox::clicked, this, &QG_MoveRotateOptions::cbFreeAngleForClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_MoveRotateOptions::onAngleEditingFinished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_MoveRotateOptions::~QG_MoveRotateOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_MoveRotateOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_MoveRotateOptions::doSaveSettings() {
    bool angleIsFree = ui->cbFreeAngle->isChecked();
    save("Angle", ui->leAngle->text());
    save("UseCurrentLayer", ui->cbCurrentLayer->isChecked());
    save("UseCurrentAttributes", ui->cbCurrentAttr->isChecked());
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
    save("MultipleCopies", ui->cbMultipleCopies->isChecked());
    save("Copies", ui->sbNumberOfCopies->value());
    save("FreeAngle", angleIsFree);
    save("SameAngleForCopies", ui->cbSameAngleForCopies->isChecked());
}

void QG_MoveRotateOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyMoveRotate*>(a);

    QString angle;

    bool useMultipleCopies;
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    int copiesNumber;
    bool sameAngle;
    bool angleIsFree;
    if (update){
        useCurrentLayer = action->isUseCurrentLayer();
        useCurrentAttributes  = action->isUseCurrentAttributes();
        copiesNumber = action->getCopiesNumber();
        keepOriginals = action->isKeepOriginals();
        useMultipleCopies = action->isUseMultipleCopies();
        sameAngle = action->isUseSameAngleForCopies();
        angleIsFree = action->isAngleFree();
        angle = fromDouble(RS_Math::rad2deg(action->getAngle()));
    }
    else{
        useCurrentLayer = loadBool("UseCurrentLayer", false);
        useCurrentAttributes = loadBool("UseCurrentAttributes", false);
        keepOriginals = loadBool("KeepOriginals", false);
        useMultipleCopies = loadBool("MultipleCopies", false);
        copiesNumber = loadInt("Copies", 1);
        sameAngle = loadBool("SameAngleForCopies", false);
        angleIsFree = loadBool("FreeAngle", true);
        angle = load("Angle", "30");
    }
    setUseMultipleCopiesToActionAndView(useMultipleCopies);
    setCopiesNumberToActionAndView(copiesNumber);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);
    setSameAngleForCopiesToActionAndView(sameAngle);
    setFreeAngleToModelAndView(angleIsFree);
    setAngleToActionAndView(angle);
}

void QG_MoveRotateOptions::setAngleToActionAndView(QString val) {
    double angle;
    if (toDoubleAngle(val, angle, 0.0, false)){
        ui->leAngle->setText(fromDouble(angle));
        action->setAngle(RS_Math::deg2rad(angle));
    }
}

void QG_MoveRotateOptions::setCopiesNumberToActionAndView(int number) {
    if (number < 1){
        number = 1;
    }
    action->setCopiesNumber(number);
    ui->sbNumberOfCopies->setValue(number);
}

void QG_MoveRotateOptions::setUseMultipleCopiesToActionAndView(bool copies) {
    action->setUseMultipleCopies(copies);
    ui->cbMultipleCopies->setChecked(copies);
    ui->sbNumberOfCopies->setEnabled(copies);
    ui->cbSameAngleForCopies->setEnabled(copies);
}

void QG_MoveRotateOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbCurrentLayer->setChecked(val);
}

void QG_MoveRotateOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void QG_MoveRotateOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void QG_MoveRotateOptions::setFreeAngleToModelAndView(bool val) {
    ui->leAngle->setEnabled(!val);
    ui->cbFreeAngle->setChecked(val);
    action->setAngleIsFree(val);
}

void QG_MoveRotateOptions::setSameAngleForCopiesToActionAndView(bool val) {
   action->setUseSameAngleForCopies(val);
   ui->cbSameAngleForCopies->setChecked(val);
}

void QG_MoveRotateOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}

void QG_MoveRotateOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void QG_MoveRotateOptions::cbMultipleCopiesClicked(bool val) {
    setUseMultipleCopiesToActionAndView(val);
}

void QG_MoveRotateOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void QG_MoveRotateOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void QG_MoveRotateOptions::cbSameAngleForCopiesClicked(bool val) {
    setSameAngleForCopiesToActionAndView(val);
}

void QG_MoveRotateOptions::cbFreeAngleForClicked(bool val) {
    setFreeAngleToModelAndView(val);
}

void QG_MoveRotateOptions::on_sbNumberOfCopies_valueChanged(int number) {
    setCopiesNumberToActionAndView(number);
}
