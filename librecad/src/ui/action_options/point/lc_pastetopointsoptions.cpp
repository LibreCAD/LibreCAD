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

#include "lc_pastetopointsoptions.h"
#include "ui_lc_pastetopointsoptions.h"
#include "rs_math.h"

LC_PasteToPointsOptions::LC_PasteToPointsOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionPasteToPoints, "Edit", "PasteToPoints")
    , ui(new Ui::LC_PasteToPointsOptions){
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_PasteToPointsOptions::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &LC_PasteToPointsOptions::onFactorEditingFinished);
    connect(ui->cbRemovePoint, &QCheckBox::clicked, this, &LC_PasteToPointsOptions::onRemovePointsClicked);
}

LC_PasteToPointsOptions::~LC_PasteToPointsOptions(){
    delete ui;
}

void LC_PasteToPointsOptions::doSaveSettings() {
    save("Angle", ui->leAngle->text());
    save("ScaleFactor", ui->leFactor->text());
    save("RemovePoints", ui->cbRemovePoint->isChecked());
}

void LC_PasteToPointsOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<LC_ActionPasteToPoints *>(a);
    QString angle;
    QString factor;
    bool removePoints;
    if (update){
        angle = fromDouble(RS_Math::rad2deg(action->getAngle()));
        factor = fromDouble(action->getScaleFactor());
        removePoints = action->isRemovePointAfterPaste();
    }
    else{
        angle = load("Angle", "0.0");
        factor = load("ScaleFactor", "1.0");
        removePoints = loadBool("RemovePoints", true);
    }
    setAngleToActionAndView(angle);
    setFactorToActionAndView(factor);
    setRemovePointsToActionAndView(removePoints);
}

void LC_PasteToPointsOptions::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void LC_PasteToPointsOptions::onFactorEditingFinished(){
    setFactorToActionAndView(ui->leFactor->text());
}

void LC_PasteToPointsOptions::onRemovePointsClicked(bool clicked){
    setRemovePointsToActionAndView(clicked);
}

void LC_PasteToPointsOptions::setRemovePointsToActionAndView(bool val) {
    ui->cbRemovePoint->setChecked(val);
    action->setRemovePointAfterPaste(val);
}

void LC_PasteToPointsOptions::setFactorToActionAndView(QString val) {
    double y;
    if (toDouble(val, y, 1.0, true)){
        action->setScaleFactor(y);
        ui->leFactor->setText(fromDouble(y));
    }
}

void LC_PasteToPointsOptions::setAngleToActionAndView(QString val) {
    double angle;
    if (toDoubleAngle(val, angle, 0.0, false)){
        action->setAngle(RS_Math::deg2rad(angle));
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_PasteToPointsOptions::languageChange() {
    ui->retranslateUi(this);
}
