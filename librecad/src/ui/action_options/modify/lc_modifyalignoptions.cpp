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

#include "lc_modifyalignoptions.h"
#include "lc_actionmodifyalign.h"
#include "ui_lc_modifyalignoptions.h"

LC_ModifyAlignOptions::LC_ModifyAlignOptions()
    : LC_ActionOptionsWidget()
    , ui(new Ui::LC_ModifyAlignOptions){
    ui->setupUi(this);

    connect(ui->cbAsGroup, &QCheckBox::toggled, this, &LC_ModifyAlignOptions::onAsGroupChanged);
    connect(ui->tbVAlignTop, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onVAlignChanged);
    connect(ui->tbVAlignMiddle, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onVAlignChanged);
    connect(ui->tbVAlignBottom, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onVAlignChanged);
    connect(ui->tbValignNone, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onVAlignChanged);
    connect(ui->tbHAlignLeft, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onHAlignChanged);
    connect(ui->tbHalignMiddle, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onHAlignChanged);
    connect(ui->tbHalignRight, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onHAlignChanged);
    connect(ui->tbHAlignNone, &QToolButton::toggled, this, &LC_ModifyAlignOptions::onHAlignChanged);
    connect(ui->cbAlignTo, &QComboBox::currentIndexChanged, this, &LC_ModifyAlignOptions::onAlignToIndexChanged);
}

LC_ModifyAlignOptions::~LC_ModifyAlignOptions(){
    delete ui;
}

bool LC_ModifyAlignOptions::checkActionRttiValid(RS2::ActionType actionType) {
    m_forAlignAction = actionType == RS2::ActionModifyAlign;
    return m_forAlignAction || (actionType == RS2::ActionModifyAlignOne);
}

QString LC_ModifyAlignOptions::getSettingsGroupName() {
    return "Modify";
}

QString LC_ModifyAlignOptions::getSettingsOptionNamePrefix() {
    return (m_forAlignAction ? "Align":"AlignOne");
}

void LC_ModifyAlignOptions::doSaveSettings() {
    int halign = getHAlignFromUI();
    int valign = getVAlignFromUI();
    save("HAlign", halign);
    save("VAlign", valign);
    save("AlignTo", ui->cbAlignTo->currentIndex());
    if (m_forAlignAction) {
        save("AsGroup", ui->cbAsGroup->isChecked());
    }
}

void LC_ModifyAlignOptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<LC_ActionModifyAlignData *>(a);
    int valign;
    int halign;
    int alignType;
    bool asGroup;
    if (update){
        valign = m_action->getVAlign();
        halign = m_action->getHAlign();
        alignType = m_action->getAlignType();
        asGroup = m_action->isAsGroup();
    }
    else{
        halign = loadInt("HAlign", 0);
        valign = loadInt("VAlign", 0);
        alignType = loadInt("AlignTo", 0);
        asGroup = loadBool("AsGroup", true);
    }
    if (m_forAlignAction) {
        setAsGroupToActionAndView(asGroup);
    }
    ui->cbAsGroup->setVisible(m_forAlignAction);
    setHAlignToActionAndView(halign);
    setVAlignToActionAndView(valign);
    setAlignTypeToActionAndView(alignType);
}

void LC_ModifyAlignOptions::onAsGroupChanged([[maybe_unused]]bool val) {
    setAsGroupToActionAndView(ui->cbAsGroup->isChecked());
}

void LC_ModifyAlignOptions::onVAlignChanged([[maybe_unused]]bool val) {
    int valign = getVAlignFromUI();
    setVAlignToActionAndView(valign);
}

void LC_ModifyAlignOptions::onHAlignChanged([[maybe_unused]]bool val) {
    int halign = getHAlignFromUI();
    setHAlignToActionAndView(halign);
}

void LC_ModifyAlignOptions::onAlignToIndexChanged([[maybe_unused]]int idx) {
    setAlignTypeToActionAndView(ui->cbAlignTo->currentIndex());
}

void LC_ModifyAlignOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_ModifyAlignOptions::setAlignTypeToActionAndView(int type) {
    m_action->setAlignType(type);
    ui->cbAlignTo->setCurrentIndex(type);
}

void LC_ModifyAlignOptions::setVAlignToActionAndView(int valign) {
    switch (valign){
      case LC_Align::Align::LEFT_TOP:{
            ui->tbVAlignTop->setChecked(true);
            break;
        }
        case LC_Align::Align::MIDDLE:{
            ui->tbVAlignMiddle->setChecked(true);
            break;
        }
        case LC_Align::Align::RIGHT_BOTTOM:{
            ui->tbVAlignBottom->setChecked(true);
            break;
        }
        default:
            ui->tbValignNone->setChecked(true);
    }
    m_action->setVAlign(valign);
}

void LC_ModifyAlignOptions::setHAlignToActionAndView(int halign) {
    switch (halign){
        case LC_Align::Align::LEFT_TOP:{
            ui->tbHAlignLeft->setChecked(true);
            break;
        }
        case LC_Align::Align::MIDDLE:{
            ui->tbHalignMiddle->setChecked(true);
            break;
        }
        case LC_Align::Align::RIGHT_BOTTOM:{
            ui->tbHalignRight->setChecked(true);
            break;
        }
        default:
            ui->tbHAlignNone->setChecked(true);
    }
    m_action->setHAlign(halign);
}

int LC_ModifyAlignOptions::getHAlignFromUI() {
    if (ui->tbHAlignLeft->isChecked()) {
        return LC_Align::Align::LEFT_TOP;
    }
    if (ui->tbHalignMiddle->isChecked()) {
        return LC_Align::Align::MIDDLE;
    }
    if (ui->tbHalignRight->isChecked()) {
        return LC_Align::Align::RIGHT_BOTTOM;
    }
    return LC_Align::Align::NONE;
}

int LC_ModifyAlignOptions::getVAlignFromUI() {
    if (ui->tbVAlignTop->isChecked()) {
        return LC_Align::Align::LEFT_TOP;
    }
    if (ui->tbVAlignMiddle->isChecked()) {
        return LC_Align::Align::MIDDLE;
    }
    if (ui->tbVAlignBottom->isChecked()) {
        return LC_Align::Align::RIGHT_BOTTOM;
    }
    return LC_Align::Align::NONE;
}

void LC_ModifyAlignOptions::setAsGroupToActionAndView(bool group) {
    m_action->setAsGroup(group);
    ui->cbAsGroup->setChecked(group);
}
