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

#include "lc_ellipse1pointoptions.h"
#include "ui_lc_ellipse1pointoptions.h"
#include "lc_actiondrawellipse1point.h"

LC_Ellipse1PointOptions::LC_Ellipse1PointOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawEllipse1Point, "Draw","Ellipse1P")
    , ui(new Ui::LC_Ellipse1PointOptions){
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Ellipse1PointOptions::onAngleEditingFinished);
    connect(ui->leMajorRadius, &QLineEdit::editingFinished, this, &LC_Ellipse1PointOptions::onMajorRadiusEditingFinished);
    connect(ui->leMinorRadius, &QLineEdit::editingFinished, this, &LC_Ellipse1PointOptions::onMinorRadiusEditingFinished);
    connect(ui->cbAngle, &QCheckBox::toggled, this, &LC_Ellipse1PointOptions::onUseAngleClicked);
    connect(ui->cbFreeAngle, &QCheckBox::toggled, this, &LC_Ellipse1PointOptions::onFreeAngleClicked);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_Ellipse1PointOptions::onDirectionChanged);
    connect(ui->rbNeg, &QRadioButton::toggled, this, &LC_Ellipse1PointOptions::onDirectionChanged);
}

LC_Ellipse1PointOptions::~LC_Ellipse1PointOptions(){
    delete ui;
}

bool LC_Ellipse1PointOptions::checkActionRttiValid(RS2::ActionType actionType) {
    return actionType == RS2::ActionDrawEllipse1Point || actionType == RS2::ActionDrawEllipseArc1Point;
}

void LC_Ellipse1PointOptions::doSaveSettings() {
    save("MajorRadius", ui->leMajorRadius->text());
    save("MinorRadius", ui->leMinorRadius->text());
    save("UseAngle", ui->cbAngle->isChecked());
    save("Angle", ui->leAngle->text());
    save("FreeAngle", ui->cbFreeAngle->isChecked());
    if (m_action->rtti() == RS2::ActionDrawEllipseArc1Point){
        save("ArcReversed", ui->rbNeg->isChecked());
    }
}

void LC_Ellipse1PointOptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<LC_ActionDrawEllipse1Point*> (a);
    if (m_action == nullptr) {
        return;
    }

    QString majorRadius;
    QString minorRadius;
    QString angle;
    bool useAngle = false;
    bool freeAngle = false;
    bool negativeDirection = false;
    bool arcAction = m_action->rtti() == RS2::ActionDrawEllipseArc1Point;
    if (update){
        majorRadius = fromDouble(m_action->getMajorRadius());
        minorRadius = fromDouble(m_action->getMinorRadius());
        angle = fromDouble(m_action->getUcsMajorAngleDegrees());
        useAngle = m_action->hasAngle();
        freeAngle = m_action->isAngleFree();
        if (arcAction){
            negativeDirection = m_action->isReversed();
        }
    }
    else{
        majorRadius = load("MajorRadius", "1.0");
        minorRadius = load("MinorRadius", "1.0");
        useAngle = loadBool("UseAngle", false);
        angle = load("Angle", "1.0");
        freeAngle = loadBool("FreeAngle", false);
        if (arcAction){
            negativeDirection = loadBool("ArcReversed", false);
        }
    }

    setMajorRadiusToActionAndView(majorRadius);
    setMinorRadiusToActionAndView(minorRadius);
    setAngleToActionAndView(angle);
    setAngleIsFreeToActionAndView(freeAngle);
    setUseAngleAngleToActionAndView(useAngle);

    ui->rbNeg->setChecked(negativeDirection);
    ui->rbPos->setChecked(!negativeDirection);

    ui->rbNeg->setVisible(arcAction);
    ui->rbPos->setVisible(arcAction);
}

void LC_Ellipse1PointOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_Ellipse1PointOptions::setMajorRadiusToActionAndView(QString val) {
    double y;
    if (toDouble(val, y, 1, true)){
        m_action->setMajorRadius(y);
        ui->leMajorRadius->setText(fromDouble(y));
    }
}

void LC_Ellipse1PointOptions::setMinorRadiusToActionAndView(QString val) {
    double y;
    if (toDouble(val, y, 1, true)){
        m_action->setMinorRadius(y);
        ui->leMinorRadius->setText(fromDouble(y));
    }
}

void LC_Ellipse1PointOptions::setAngleToActionAndView(QString val) {
    double y;
    if (toDoubleAngleDegrees(val, y, 0, false)){
        m_action->setUcsMajorAngleDegrees(y);
        ui->leAngle->setText(fromDouble(y));
    }
}

void LC_Ellipse1PointOptions::setAngleIsFreeToActionAndView(bool val) {
    m_action->setAngleFree(val);
    ui->cbFreeAngle->setChecked(val);
    ui->leAngle->setEnabled(!val);
}

void LC_Ellipse1PointOptions::setUseAngleAngleToActionAndView(bool val) {
    ui->cbAngle->setChecked(val);
    m_action->setHasAngle(val);
    ui->leAngle->setEnabled(val && !ui->cbFreeAngle->isChecked());
    ui->cbFreeAngle->setEnabled(val);
}

void LC_Ellipse1PointOptions::onAngleEditingFinished() {
    setAngleToActionAndView(ui->leAngle->text());
}

void LC_Ellipse1PointOptions::onMajorRadiusEditingFinished() {
    setMajorRadiusToActionAndView(ui->leMajorRadius->text());
}

void LC_Ellipse1PointOptions::onMinorRadiusEditingFinished() {
    setMinorRadiusToActionAndView(ui->leMinorRadius->text());
}

void LC_Ellipse1PointOptions::onUseAngleClicked([[maybe_unused]]bool val) {
    setUseAngleAngleToActionAndView(ui->cbAngle->isChecked());
}

void LC_Ellipse1PointOptions::onFreeAngleClicked([[maybe_unused]]bool val) {
    setAngleIsFreeToActionAndView(ui->cbFreeAngle->isChecked());
}

void LC_Ellipse1PointOptions::onDirectionChanged([[maybe_unused]] bool val) {
    bool negative = ui->rbNeg->isChecked();
    m_action->setReversed(negative);
}
