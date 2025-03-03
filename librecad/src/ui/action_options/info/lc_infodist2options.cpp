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

#include "lc_infodist2options.h"
#include "ui_lc_infodist2options.h"

LC_InfoDist2Options::LC_InfoDist2Options()
    : LC_ActionOptionsWidget(nullptr)
    , ui(new Ui::LC_InfoDist2Options){
    ui->setupUi(this);
    connect(ui->cbOnEntity, &QCheckBox::clicked, this, &LC_InfoDist2Options::onOnEntityClicked);
}

LC_InfoDist2Options::~LC_InfoDist2Options(){
    delete ui;
    action = nullptr;
}

void LC_InfoDist2Options::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<RS_ActionInfoDist2 *>(a);

    bool onEntity;
    if (update){
        onEntity = action->isUseNearestPointOnEntity();
    } else {
        onEntity = loadBool("NearestIsOnEntity", true);
    }
    setOnEntitySnapToActionAndView(onEntity);
}

QString LC_InfoDist2Options::getSettingsOptionNamePrefix(){
    return "InfoDist2";
}

void LC_InfoDist2Options::doSaveSettings(){
    save("NearestIsOnEntity", ui->cbOnEntity->isChecked());
}

void LC_InfoDist2Options::onOnEntityClicked([[maybe_unused]]bool value){
    if (action != nullptr){
        setOnEntitySnapToActionAndView(ui->cbOnEntity->isChecked());
    }
}

void LC_InfoDist2Options::setOnEntitySnapToActionAndView(bool value){
    action->setUseNearestPointOnEntity(value);
    ui->cbOnEntity->setChecked(value);
}

void LC_InfoDist2Options::languageChange(){
    ui->retranslateUi(this);
}

bool LC_InfoDist2Options::checkActionRttiValid(RS2::ActionType actionType){
        return actionType ==RS2::ActionInfoDistEntity2Point || actionType == RS2::ActionInfoDistPoint2Entity;
}
