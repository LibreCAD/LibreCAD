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
#include<cmath>

#include "qg_arctangentialoptions.h"
#include "rs_actiondrawarctangential.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_arctangentialoptions.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

/*
 *  Constructs a QG_ArcTangentialOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ArcTangentialOptions::QG_ArcTangentialOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawArcTangential, "Draw", "ArcTangential")
    , ui(new Ui::Ui_ArcTangentialOptions{}){
    ui->setupUi(this);
    connect(ui->rbRadius, &QRadioButton::clicked, this, &QG_ArcTangentialOptions::onRadiusClicked);
    connect(ui->rbAngle, &QRadioButton::clicked, this, &QG_ArcTangentialOptions::onAngleClicked);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &QG_ArcTangentialOptions::onRadiusEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_ArcTangentialOptions::onAngleEditingFinished);
}

QG_ArcTangentialOptions::~QG_ArcTangentialOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ArcTangentialOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_ArcTangentialOptions::doSaveSettings(){
    bool byRadius = ui->rbRadius->isChecked();
    save("ByRadius", byRadius);
    if (byRadius){
        save("Radius", ui->leRadius->text());
    }
    else {
        save("Angle", ui->leAngle->text());
    }
}

void QG_ArcTangentialOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<RS_ActionDrawArcTangential *>(a);

    QString radius;
    QString angle;
    bool byRadius;
    if (update){
        radius = fromDouble(action->getRadius());
        angle = fromDouble(RS_Math::rad2deg(action->getAngle()));
        byRadius = action->getByRadius();
    } else {
        radius = load("Radius", "1.0");
        angle = load("Angle", "90.0");
        byRadius = loadBool("ByRadius", true);
    }
    setByRadiusToActionAndView(byRadius);
    setRadiusToActionAndView(radius);
    setAngleToActionAndView(angle);
}

void QG_ArcTangentialOptions::setRadiusToActionAndView(const QString& s) {
    double radius;
    if (toDouble(s, radius, 1.0, true)){
        action->setRadius(radius);
        ui->leRadius->setText(fromDouble(radius));
    }
}

void QG_ArcTangentialOptions::setAngleToActionAndView(const QString& s) {
    ui->leAngle->setText(s);
    double angleDegree;
    if (toDoubleAngle(s, angleDegree, 1.0, true)){
        double angleRad = RS_Math::correctAngle(RS_Math::deg2rad(angleDegree));
        if(angleRad <RS_TOLERANCE_ANGLE || angleRad  + RS_TOLERANCE_ANGLE > 2. * M_PI)
            angleRad =M_PI; // can not do full circle
        action->setAngle(angleRad);
        angleDegree = RS_Math::rad2deg(angleRad);
        ui->leAngle->setText(fromDouble(angleDegree));
    }
}

void QG_ArcTangentialOptions::setByRadiusToActionAndView(bool byRadius) {
    action->setByRadius(byRadius);
    ui->rbRadius->setChecked(byRadius);
    ui->rbAngle->setChecked(!byRadius);
    ui->leRadius->setEnabled(byRadius);
    ui->leAngle->setEnabled(!byRadius);
}

void QG_ArcTangentialOptions::onRadiusEditingFinished(){
    setRadiusToActionAndView(ui->leRadius->text());
}

void QG_ArcTangentialOptions::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_ArcTangentialOptions::onRadiusClicked(bool /*checked*/){
    setByRadiusToActionAndView(true);
}

void QG_ArcTangentialOptions::onAngleClicked(bool /*checked*/){
    setByRadiusToActionAndView(false);
}

// fixme - add label that will show current arc radius or angle (on preview)

void QG_ArcTangentialOptions::updateRadius(double d){
    ui->leRadius->setText(fromDouble(d));
}

void QG_ArcTangentialOptions::updateAngle(double d){
    ui->leAngle->setText(fromDouble(d));
}
