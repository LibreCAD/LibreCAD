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
#include "lc_actionmodifyduplicate.h"
#include "ui_lc_duplicateoptions.h"

LC_DuplicateOptions::LC_DuplicateOptions():
    LC_ActionOptionsWidgetBase(RS2::ActionModifyDuplicate, "Modify","Duplicate"),
    ui(new Ui::LC_DuplicateOptions),
    m_action(nullptr){
    ui->setupUi(this);
    connect(ui->leOffsetX, &QLineEdit::editingFinished, this, &LC_DuplicateOptions::onOffsetXEditingFinished);
    connect(ui->leOffsetY, &QLineEdit::editingFinished, this, &LC_DuplicateOptions::onOffsetYEditingFinished);
    connect(ui->cbInPlace, &QCheckBox::clicked, this, &LC_DuplicateOptions::onInPlaceClicked);
    connect(ui->cbPen, &QComboBox::currentIndexChanged, this, &LC_DuplicateOptions::onPenModeIndexChanged);
    connect(ui->cbLayer, &QComboBox::currentIndexChanged, this, &LC_DuplicateOptions::onLayerModeIndexChanged);

    pickDistanceSetup("offsetX", ui->tbPickOffsetX, ui->leOffsetX);
    pickDistanceSetup("offsetY", ui->tbPickOffsetY, ui->leOffsetY);
}

LC_DuplicateOptions::~LC_DuplicateOptions(){
    delete ui;
    m_action = nullptr;
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
    m_action = dynamic_cast<LC_ActionModifyDuplicate *>(a);
    QString ofX;
    QString ofY;
    bool inplace;
    int penMode;
    int layerMode;
    if (update){
        ofX = fromDouble(m_action->getOffsetX());
        ofY = fromDouble(m_action->getOffsetY());
        inplace = m_action->isDuplicateInPlace();
        penMode = m_action->getPenMode();
        layerMode = m_action->getLayerMode();
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
    if (m_action != nullptr){
        setPenModeToActionAndView(mode);
    }
}

void LC_DuplicateOptions::onLayerModeIndexChanged(int mode){
    if (m_action != nullptr){
        setLayerModeToActionAndeView(mode);
    }
}

void LC_DuplicateOptions::setOffsetXToActionAndView(const QString &val){
    double value;
    if (toDouble(val, value, 0, false)){
        m_action->setOffsetX(value);
        ui->leOffsetX->setText(fromDouble(value));
    }
}

void LC_DuplicateOptions::setOffsetYToActionAndView(const QString &val){
    double value;
    if (toDouble(val, value, 0, false)){
        m_action->setOffsetY(value);
        ui->leOffsetY->setText(fromDouble(value));
    }
}

void LC_DuplicateOptions::setInPlaceDuplicateToActionAndView(bool inplace){
    ui->leOffsetX->setEnabled(!inplace);
    ui->leOffsetY->setEnabled(!inplace);
    ui->tbPickOffsetX->setEnabled(!inplace);
    ui->tbPickOffsetY->setEnabled(!inplace);
    ui->cbPen->setEnabled(!inplace);
    ui->cbLayer->setEnabled(!inplace);
    m_action->setDuplicateInPlace(inplace);
    ui->cbInPlace->setChecked(inplace);
}

void LC_DuplicateOptions::setPenModeToActionAndView(int mode){
    m_action->setPenMode(mode);
    ui->cbPen->setCurrentIndex(mode);
}

void LC_DuplicateOptions::setLayerModeToActionAndeView(int mode){
    m_action->setLayerMode(mode);
    ui->cbLayer->setCurrentIndex(mode);
}
