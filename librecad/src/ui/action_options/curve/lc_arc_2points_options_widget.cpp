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

#include "lc_arc_2points_options_widget.h"

#include "lc_action_draw_arc_2points_base.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_arc_2points_options_widget.h"

LC_Arc2PointsOptionsWidget::LC_Arc2PointsOptionsWidget(const int actionType)
    : ui(new Ui::LC_Arc2PointsOptionsWidget){
    ui->setupUi(this);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_Arc2PointsOptionsWidget::onDirectionChanged);
    connect(ui->rbNeg,  &QRadioButton::toggled, this, &LC_Arc2PointsOptionsWidget::onDirectionChanged);
    connect(ui->leValue, &QLineEdit::editingFinished, this, &LC_Arc2PointsOptionsWidget::onValueChanged);

    ui->lRadius->setVisible(false);
    ui->lLength->setVisible(false);
    ui->lHeight->setVisible(false);
    ui->lAngle->setVisible(false);
    ui->tbPickAngle->setVisible(false);
    ui->tbPickDist->setVisible(m_interactiveInputControlsVisible);

    switch (actionType){
        case RS2::ActionDrawArc2PRadius: {
            updateTooltip(ui->lRadius);
            ui->tbPickDist->setToolTip(tr("Pick radius from drawing"));
            pickDistanceSetup("radius", ui->tbPickDist, ui->leValue);
            break;
        }
        case RS2::ActionDrawArc2PHeight:{
            updateTooltip(ui->lHeight);
            ui->tbPickDist->setToolTip(tr("Pick height from drawing"));
            pickDistanceSetup("height", ui->tbPickDist, ui->leValue);
            break;
        }
        case RS2::ActionDrawArc2PLength:{
            updateTooltip(ui->lLength);
            ui->tbPickDist->setToolTip(tr("Pick length from drawing"));
            pickDistanceSetup("length", ui->tbPickDist, ui->leValue);
            break;
        }
        case RS2::ActionDrawArc2PAngle:{
            updateTooltip(ui->lAngle);
            ui->tbPickDist->setToolTip(tr("Pick angle from drawing"));
            ui->tbPickDist->setVisible(false);
            ui->tbPickAngle->setVisible(m_interactiveInputControlsVisible);
            pickAngleSetup("angle", ui->tbPickAngle, ui->leValue);
            break;
        }
        default:
            break;
    }
}

void LC_Arc2PointsOptionsWidget::updateTooltip( QLabel *label) const {
    const QString toolTip = label->toolTip();
    label->setToolTip("");
    ui->leValue->setToolTip(toolTip);
    label->setVisible(true);
}

LC_Arc2PointsOptionsWidget::~LC_Arc2PointsOptionsWidget(){
    delete ui;
}

void LC_Arc2PointsOptionsWidget::languageChange(){
    ui->retranslateUi(this);
}


void LC_Arc2PointsOptionsWidget::doUpdateByAction(RS_ActionInterface *a) {
    m_action = static_cast<LC_ActionDrawArc2PointsBase *>(a);

    const bool reversed = m_action->isReversed();
        double param = m_action->getParameter();
        if (m_action->rtti() == RS2::ActionDrawArc2PAngle){
            param = RS_Math::rad2deg(param);
        }
        const QString parameter = fromDouble(param);

    LC_GuardedSignalsBlocker({ui->rbNeg, ui->rbPos, ui->leValue});

    ui->rbNeg->setChecked(reversed);
    ui->leValue->setText(parameter);
}

void LC_Arc2PointsOptionsWidget::onValueChanged() {
    const auto val = ui->leValue->text();
    double param;
    if (m_action->rtti() == RS2::ActionDrawArc2PAngle){
        if (toDoubleAngleDegrees(val, param, 1.0, true)){
            param = RS_Math::deg2rad(param);
            m_action->setParameter(param);
        }
    }
    else if (toDouble(val, param, 1.0, true)){
        m_action->setParameter(param);
    }
    m_action->updateOptions();
}

void LC_Arc2PointsOptionsWidget::onDirectionChanged(bool /*pos*/){
    m_action->setReversed(ui->rbNeg->isChecked());
    m_action->updateOptions();
}
