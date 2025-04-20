/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_ucssetoptions.h"
#include "lc_actionucscreate.h"
#include "ui_lc_ucssetoptions.h"

LC_UCSSetOptions::LC_UCSSetOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionUCSCreate, "UCS","Set")
    , ui(new Ui::LC_UCSSetOptions){
    ui->setupUi(this);
    connect(ui->cbFreeAngle, &QCheckBox::clicked, this, &LC_UCSSetOptions::cbFreeAngleClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_UCSSetOptions::onAngleEditingFinished);
}

LC_UCSSetOptions::~LC_UCSSetOptions(){
    delete ui;
}

void LC_UCSSetOptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<LC_ActionUCSCreate *>(a);
    QString angle;
    bool freeAngle = false;
    if (update){
        freeAngle = !m_action->isFixedAngle();
        angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
    }
    else{
        freeAngle = loadBool("AngleIsFree", true);
        angle = load("Angle", "0.0");
    }
    setAngleToActionAndView(angle);
    setAngleIsFreeToActionAndView(freeAngle);
}

void LC_UCSSetOptions::doSaveSettings() {
    save("Angle", ui->leAngle->text());
    save("AngleIsFree", ui->cbFreeAngle->isChecked());
}

void LC_UCSSetOptions::updateUI(int mode) {
   if (mode == 1){
       QString angle = fromDouble(RS_Math::rad2deg(m_action->getCurrentAngle()));

       ui->leAngle->blockSignals(true);
       ui->leAngle->setText(angle);
       ui->leAngle->blockSignals(false);

       ui->leAngle->update();
   }
}

void LC_UCSSetOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_UCSSetOptions::setAngleToActionAndView(QString val) {
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        const QString &factorStr = fromDouble(angle);
        ui->leAngle->setText(factorStr);
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
}

void LC_UCSSetOptions::setAngleIsFreeToActionAndView(bool val) {
    ui->cbFreeAngle->setChecked(val);
    m_action->setFixedAngle(!val);
    if (val){
        ui->leAngle->setEnabled(false);
    }
    else{
        ui->leAngle->setEnabled(true);
    }
}

void LC_UCSSetOptions::onAngleEditingFinished() {
    setAngleToActionAndView(ui->leAngle->text());
}

void LC_UCSSetOptions::cbFreeAngleClicked(bool val) {
    setAngleIsFreeToActionAndView(val);
}
