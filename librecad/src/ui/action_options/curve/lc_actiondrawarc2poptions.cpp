/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_actiondrawarc2poptions.h"
#include "lc_actiondrawarc2pointsbase.h"
#include "ui_lc_actiondrawarc2poptions.h"

LC_ActionDrawArc2POptions::LC_ActionDrawArc2POptions(int actionType)
    : LC_ActionOptionsWidget()
    , ui(new Ui::LC_ActionDrawArc2POptions){
    ui->setupUi(this);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_ActionDrawArc2POptions::onDirectionChanged);
    connect(ui->rbNeg,  &QRadioButton::toggled, this, &LC_ActionDrawArc2POptions::onDirectionChanged);
    connect(ui->leValue, &QLineEdit::editingFinished, this, &LC_ActionDrawArc2POptions::onValueChanged);
    m_supportedActionType = actionType;

    ui->lRadius->setVisible(false);
    ui->lLength->setVisible(false);
    ui->lHeight->setVisible(false);
    ui->lAngle->setVisible(false);

    switch (actionType){
        case RS2::ActionDrawArc2PRadius: {
            updateTooltip(ui->lRadius);
            m_optionNamePrefix = "Arc2PRadius";
            break;
        }
        case RS2::ActionDrawArc2PHeight:{
            updateTooltip(ui->lHeight);
            m_optionNamePrefix = "Arc2PHeight";
            break;
        }
        case RS2::ActionDrawArc2PLength:{
            updateTooltip(ui->lLength);
            m_optionNamePrefix = "Arc2PLength";
            break;
        }
        case RS2::ActionDrawArc2PAngle:{
            updateTooltip(ui->lAngle);
            m_optionNamePrefix = "Arc2PAngle";
            break;
        }
        default:
            break;
    }

}

void LC_ActionDrawArc2POptions::updateTooltip( QLabel *label) const {
    QString toolTip;
    toolTip = label->toolTip();
    label->setToolTip("");
    ui->leValue->setToolTip(toolTip);
    label->setVisible(true);
}

LC_ActionDrawArc2POptions::~LC_ActionDrawArc2POptions(){
    delete ui;
}

void LC_ActionDrawArc2POptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_ActionDrawArc2POptions::doSaveSettings(){
    save("Reversed",  ui->rbNeg->isChecked());
    save("Parameter", ui->leValue->text());
}

bool LC_ActionDrawArc2POptions::checkActionRttiValid(RS2::ActionType actionType) {
    return actionType == m_supportedActionType;
}

void LC_ActionDrawArc2POptions::setReversedToActionAndView(bool reversed){
    ui->rbNeg->setChecked(reversed);
    m_action->setReversed(reversed);
}

/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/
void LC_ActionDrawArc2POptions::onDirectionChanged(bool /*pos*/){
    setReversedToActionAndView( ui->rbNeg->isChecked());
}
void LC_ActionDrawArc2POptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<LC_ActionDrawArc2PointsBase *>(a);
    bool reversed;
    QString parameter;
    if (update){
        reversed = m_action->isReversed();
        double param = m_action->getParameter();
        if (m_supportedActionType == RS2::ActionDrawArc2PAngle){
            param = RS_Math::rad2deg(param);
        }
        parameter = fromDouble(param);
    }
    else{
        reversed = loadBool("Reversed", false);
        parameter = load("Parameter", "1.0");
    }
    setReversedToActionAndView(reversed);
    setParameterToActionAndView(parameter);
}

QString LC_ActionDrawArc2POptions::getSettingsOptionNamePrefix() {
    return m_optionNamePrefix;
}

void LC_ActionDrawArc2POptions::onValueChanged() {
    setParameterToActionAndView(ui->leValue->text());
}

void LC_ActionDrawArc2POptions::setParameterToActionAndView(QString val) {
    double param;
    if (m_supportedActionType == RS2::ActionDrawArc2PAngle){
        if (toDoubleAngleDegrees(val, param, 1.0, true)){
            ui->leValue->setText(fromDouble(param));
            param = RS_Math::deg2rad(param);
            m_action->setParameter(param);
        }
    }
    else if (toDouble(val, param, 1.0, true)){
        m_action->setParameter(param);
        ui->leValue->setText(fromDouble(param));
    }
}
