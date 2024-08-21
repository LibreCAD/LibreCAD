/****************************************************************************
**
* Options widget for Angle Line from line action.

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

#include "lc_lineanglereloptions.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "ui_lc_lineanglereloptions.h"


LC_LineAngleRelOptions::LC_LineAngleRelOptions() :
    LC_ActionOptionsWidget(nullptr),
    ui(std::make_unique<Ui::LC_LineAngleRelOptions>())
{
    ui->setupUi(this);

    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onLengthEditingFinished);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onOffsetEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onAngleEditingFinished);
    connect(ui->cbRelativeAngle, SIGNAL(clicked(bool)), this, SLOT(onAngleRelatedClicked(bool)));
    connect(ui->cbDivide, SIGNAL(clicked(bool)), this, SLOT(onDivideClicked(bool)));
    connect(ui->cbFree, SIGNAL(clicked(bool)), this, SLOT(onFreeLengthClicked(bool)));
    connect(ui->cbTickSnapMode, SIGNAL(currentIndexChanged(int)), SLOT(onTickSnapModeIndexChanged(int)));
    connect(ui->cbLineSnapMode, SIGNAL(currentIndexChanged(int)), SLOT(onLineSnapModeIndexChanged(int)));
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onDistanceEditingFinished);
}

LC_LineAngleRelOptions::~LC_LineAngleRelOptions(){
    action = nullptr;
}

bool LC_LineAngleRelOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLineAngleRel || actionType == RS2::ActionDrawLineOrthogonalRel;
}

void LC_LineAngleRelOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionDrawLineAngleRel*>(a);
    fixedAngle = a->rtti()==RS2::ActionDrawLineOrthogonalRel;
        QString length;
        QString offset;
        QString angle;
        int lineSnapMode = 0;
        int tickSnapMode = 0;
        bool angleIsRelative = false;
        bool lengthIsFree = false;
        bool divide = false;
        QString distance;
        if (update) {
            length = fromDouble(action->getTickLength());
            offset = fromDouble(action->getTickOffset());
            angle = fromDouble(action->getTickAngle());
            lineSnapMode = action->getLineSnapMode();
            tickSnapMode = action->getTickSnapMode();
            angleIsRelative = action->isAngleRelative();
            lengthIsFree = action->isLengthFree();
            divide = action->isDivideLine();
            distance = QString("%1").arg(action->getSnapDistance());
        } else {
            length = load("Length", "1.0");
            offset = load("Offset", "1.0");
            if (!fixedAngle){
                angle = load("Angle", "1.0");
                angleIsRelative = loadBool("AngleIsRelative", true);
            }
            lengthIsFree = loadBool("LengthIsFree", true);
            lineSnapMode = loadInt("LineSnapMode", 0);
            tickSnapMode = loadInt("TickSnapMode", 1);
            divide = loadBool("DoDivide", false);
            distance = load("SnapDistance", "0.0");
        }
        ui->leAngle->setVisible(!fixedAngle);
        ui->lblAngle->setVisible(!fixedAngle);
        ui->cbRelativeAngle->setVisible(!fixedAngle);

        setLengthIsFreeToActionAndView(lengthIsFree);
        setLengthToActionAndView(length);
        setOffsetToActionAndView(offset);
        if (!fixedAngle){
            setAngleToActionAndView(angle);
            setAngleIsRelativeToActionAndView(angleIsRelative);
        }
        setLineSnapModeToActionAndView(lineSnapMode);
        setTickSnapModeToActionAndView(tickSnapMode);
        setDivideToActionAndView(divide);
        setDistanceToActionAndView(distance);
}


QString LC_LineAngleRelOptions::getSettingsOptionNamePrefix(){
    return fixedAngle ? "LineOrthogonalRel" : "LineAngleRel";
}

void LC_LineAngleRelOptions::doSaveSettings(){
    save("Length", ui->leLength->text());
    if (!fixedAngle){
        save("Angle", ui->leAngle->text());
        save("AngleIsRelative", ui->cbRelativeAngle->isChecked());
    }
    save("LengthIsFree", ui->cbFree->isChecked());
    save("Offset", ui->leOffset->text());
    save("LineSnapMode", ui->cbLineSnapMode->currentIndex());
    save("TickSnapMode", ui->cbTickSnapMode->currentIndex());
    save("DoDivide", ui->cbDivide->isChecked());
    save("SnapDistance", ui->leDistance->text());
}

void LC_LineAngleRelOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineAngleRelOptions::setAngleToActionAndView(const QString &expr){
    double angle = 0.;
    if (toDoubleAngle(expr, angle, 1.0, false)){
        action->setTickAngle(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_LineAngleRelOptions::setLengthToActionAndView(const QString& val){
    double value = 0.;
    if (toDouble(val, value, 1.0, false)){
        action->setTickLength(value);
        ui->leLength->setText(fromDouble(value));
    }
}

void LC_LineAngleRelOptions::setDistanceToActionAndView(const QString& val){
    double value = 0.;
    if (toDouble(val, value, 0.0, false)){
        action->setSnapDistance(value);
        ui->leDistance->setText(fromDouble(value));
    }
}

void LC_LineAngleRelOptions::setOffsetToActionAndView(const QString& val){
    double value = 0.;
    if (toDouble(val, value, 0.0, false)){
        action->setTickOffset(value);
        ui->leOffset->setText(fromDouble(value));
    }
}

void LC_LineAngleRelOptions::setAngleIsRelativeToActionAndView(bool relative){
    action->setAngleIsRelative(relative);
    ui->cbRelativeAngle->setChecked(relative);
}

void LC_LineAngleRelOptions::setDivideToActionAndView(bool divide){
    action->setDivideLine(divide);
    ui->cbRelativeAngle->setChecked(divide);
}

void LC_LineAngleRelOptions::setLengthIsFreeToActionAndView(bool free){
    action->setLengthIsFree(free);
    ui->cbFree->setChecked(free);
}

void LC_LineAngleRelOptions::setTickSnapModeToActionAndView(int mode){
    action->setTickSnapMode(mode);
    ui->cbTickSnapMode->setCurrentIndex(mode);
}

void LC_LineAngleRelOptions::setLineSnapModeToActionAndView(int mode){
    action->setLineSnapMode(mode);
    ui->cbLineSnapMode->setCurrentIndex(mode);
    bool notFreeSnap = mode != 0;
    ui->lblDistance->setVisible(notFreeSnap);
    ui->leDistance->setVisible(notFreeSnap);
}

void LC_LineAngleRelOptions::onLengthEditingFinished(){
    if (action != nullptr){
        const QString &expr = ui->leLength->text();
        setLengthToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onDistanceEditingFinished(){
    if (action != nullptr){
        const QString &expr = ui->leDistance->text();
        setDistanceToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onOffsetEditingFinished(){
    if (action != nullptr){
        const QString &expr = ui->leOffset->text();
        setOffsetToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onAngleEditingFinished(){
    if (action != nullptr){
        const QString &expr = ui->leAngle->text();
        setAngleToActionAndView(expr);
    }
}

void LC_LineAngleRelOptions::onLineSnapModeIndexChanged(int index){
    if (action != nullptr){
        setLineSnapModeToActionAndView(index);
    }
}

void LC_LineAngleRelOptions::onTickSnapModeIndexChanged(int index){
    if (action != nullptr){
        setTickSnapModeToActionAndView(index);
    }
}

void LC_LineAngleRelOptions::onFreeLengthClicked(bool clicked){
    if (action != nullptr){
        setLengthIsFreeToActionAndView(clicked);
    }
}

void LC_LineAngleRelOptions::onAngleRelatedClicked(bool clicked){
    if (action != nullptr){
        setAngleIsRelativeToActionAndView(clicked);
    }
}

void LC_LineAngleRelOptions::onDivideClicked(bool clicked){
    if (action != nullptr){
        setDivideToActionAndView(clicked);
    }
}
