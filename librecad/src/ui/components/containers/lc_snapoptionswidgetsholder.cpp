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

#include "lc_snapoptionswidgetsholder.h"
#include "ui_lc_snapoptionswidgetsholder.h"

LC_SnapOptionsWidgetsHolder::LC_SnapOptionsWidgetsHolder(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LC_SnapOptionsWidgetsHolder){
    ui->setupUi(this);

    ui->snapDistanceOptions->hide();
    ui->snapMiddleOptions->hide();
    ui->lineLeft->hide();
    ui->lineRight->hide();
}

LC_SnapOptionsWidgetsHolder::~LC_SnapOptionsWidgetsHolder(){
    delete ui;
}

void LC_SnapOptionsWidgetsHolder::languageChange(){
    ui->retranslateUi(this);
}

void LC_SnapOptionsWidgetsHolder::hideSnapOptions(){
    ui->snapDistanceOptions->hide();
    ui->snapMiddleOptions->hide();
    hideSeparator();
    updateParent();
}

void LC_SnapOptionsWidgetsHolder::hideSeparator() {
    if (widgetOnLeftWithinContainer){
        ui->lineRight->hide();
    }
    else{
        ui->lineLeft->hide();
    }
}

void LC_SnapOptionsWidgetsHolder::showSeparator() {
    if (widgetOnLeftWithinContainer){
        ui->lineRight->show();
    }
    else{
        ui->lineLeft->show();
    }
    updateParent();
}

void LC_SnapOptionsWidgetsHolder::showSnapMiddleOptions(int* middlePoints, bool on){
    if (on){
        ui->snapMiddleOptions->useMiddlePointsValue(middlePoints);
        ui->snapMiddleOptions->doShow();
        showSeparator();
    }
    else{
        ui->snapMiddleOptions->hide();
        if (!ui->snapDistanceOptions->isVisible()){
            hideSeparator();
        }
    }
    updateParent();
}

void LC_SnapOptionsWidgetsHolder::updateParent() const {
    auto* parentWidget = dynamic_cast<QWidget *>(parent());
    parentWidget->update();
}

void LC_SnapOptionsWidgetsHolder::showSnapDistOptions(double* dist, bool on){
    if (on){
        ui->snapDistanceOptions->useSnapDistanceValue(dist);
        ui->snapDistanceOptions->doShow();
        showSeparator();
    }
    else{
        ui->snapDistanceOptions->hide();
        if (!ui->snapMiddleOptions->isVisible()){
            hideSeparator();
        }
    }
}

void LC_SnapOptionsWidgetsHolder::updateBy(LC_SnapOptionsWidgetsHolder *other) {
    // ugly method used to restore state of snap options if switch of settings occured...
    // this allows to change location of snap options without the application restart
    bool snapDistanceOn = other->ui->snapDistanceOptions->isVisible();
    if (snapDistanceOn) {
        double* dist = other->ui->snapDistanceOptions->getDistanceValue();
        ui->snapDistanceOptions->useSnapDistanceValue(dist);
        ui->snapDistanceOptions->doShow();
        showSeparator();
    }
    bool snapMiddleOn = other->ui->snapMiddleOptions->isVisible();
    if (snapMiddleOn){
        int* num = other->ui->snapMiddleOptions->getMiddlePointsValue();
        ui->snapMiddleOptions->useMiddlePointsValue(num);
        ui->snapMiddleOptions->doShow();
        showSeparator();
    }
    updateParent();
    other->hideSnapOptions();
}
