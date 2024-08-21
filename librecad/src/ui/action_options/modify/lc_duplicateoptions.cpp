/****************************************************************************
**
* Options widget for "Duplicate" action.

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
#include "lc_duplicateoptions.h"
#include "ui_lc_duplicateoptions.h"

LC_DuplicateOptions::LC_DuplicateOptions():
    LC_ActionOptionsWidgetBase(RS2::ActionModifyDuplicate, "Modify","Duplicate"),
    ui(new Ui::LC_DuplicateOptions),
    action(nullptr){
    ui->setupUi(this);
    connect(ui->leOffsetX, &QLineEdit::editingFinished, this, &LC_DuplicateOptions::onOffsetXEditingFinished);
    connect(ui->leOffsetY, &QLineEdit::editingFinished, this, &LC_DuplicateOptions::onOffsetYEditingFinished);
    connect(ui->cbInPlace, SIGNAL(clicked(bool)), this, SLOT(onInPlaceClicked(bool)));
    connect(ui->cbPen, SIGNAL(currentIndexChanged(int)), SLOT(onPenModeIndexChanged(int)));
    connect(ui->cbLayer, SIGNAL(currentIndexChanged(int)), SLOT(onLayerModeIndexChanged(int)));
}

LC_DuplicateOptions::~LC_DuplicateOptions(){
    delete ui;
    action = nullptr;
}

void LC_DuplicateOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_DuplicateOptions::doSaveSettings(){
    save("OffsetX", ui->leOffsetX->text());
    save("OffsetY", ui->leOffsetY->text());
    save("InPlace", ui->cbInPlace->isChecked());
    save("PenMode", ui->cbPen->currentIndex());
    save("LayerMode", ui->cbLayer->currentIndex());
}

void LC_DuplicateOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionModifyDuplicate *>(a);
    QString ofX;
    QString ofY;
    bool inplace;
    int penMode;
    int layerMode;
    if (update){
        ofX = fromDouble(action->getOffsetX());
        ofY = fromDouble(action->getOffsetY());
        inplace = action->isDuplicateInPlace();
        penMode = action->getPenMode();
        layerMode = action->getLayerMode();
    }
    else{
        ofX = load("OffsetX", "0");
        ofY = load("OffsetY", "0");
        inplace = loadBool("InPlace", true);
        penMode = loadInt("PenMode", 0);
        layerMode = loadInt("LayerMode", 0);

    }
    setOffsetXToActionAndView(ofX);
    setOffsetYToActionAndView(ofY);
    setInPlaceDuplicateToActionAndView(inplace);
    setPenModeToActionAndView(penMode);
    setLayerModeToActionAndeView(layerMode);
}

void LC_DuplicateOptions::onOffsetXEditingFinished(){
    const QString &expr = ui->leOffsetX->text();
    setOffsetXToActionAndView(expr);
}

void LC_DuplicateOptions::onInPlaceClicked(bool value){
    setInPlaceDuplicateToActionAndView(value);
}

void LC_DuplicateOptions::onOffsetYEditingFinished(){
    const QString &expr = ui->leOffsetY->text();
    setOffsetYToActionAndView(expr);
}

void LC_DuplicateOptions::onPenModeIndexChanged(int mode){
    if (action != nullptr){
        setPenModeToActionAndView(mode);
    }
}

void LC_DuplicateOptions::onLayerModeIndexChanged(int mode){
    if (action != nullptr){
        setLayerModeToActionAndeView(mode);
    }
}

void LC_DuplicateOptions::setOffsetXToActionAndView(const QString &val){
    double value;
    if (toDouble(val, value, 0, false)){
        action->setOffsetX(value);
        ui->leOffsetX->setText(fromDouble(value));
    }
}

void LC_DuplicateOptions::setOffsetYToActionAndView(const QString &val){
    double value;
    if (toDouble(val, value, 0, false)){
        action->setOffsetY(value);
        ui->leOffsetY->setText(fromDouble(value));
    }
}

void LC_DuplicateOptions::setInPlaceDuplicateToActionAndView(bool inplace){
    ui->leOffsetX->setEnabled(!inplace);
    ui->leOffsetY->setEnabled(!inplace);
    ui->cbPen->setEnabled(!inplace);
    ui->cbLayer->setEnabled(!inplace);
    action->setDuplicateInPlace(inplace);
    ui->cbInPlace->setChecked(inplace);
}

void LC_DuplicateOptions::setPenModeToActionAndView(int mode){
    action->setPenMode(mode);
    ui->cbPen->setCurrentIndex(mode);
}

void LC_DuplicateOptions::setLayerModeToActionAndeView(int mode){
    action->setLayerMode(mode);
    ui->cbLayer->setCurrentIndex(mode);
}
