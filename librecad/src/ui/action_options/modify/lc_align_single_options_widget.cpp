/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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


#include "lc_align_single_options_widget.h"

#include "lc_action_modify_align_single.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_align_single_options_widget.h"

LC_AlignSingleOptionsWidget::LC_AlignSingleOptionsWidget()
    : ui(new Ui::LC_AlignSingleOptionsWidget) {
    ui->setupUi(this);


    connect(ui->tbVAlignTop, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onVAlignChanged);
    connect(ui->tbVAlignMiddle, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onVAlignChanged);
    connect(ui->tbVAlignBottom, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onVAlignChanged);
    connect(ui->tbValignNone, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onVAlignChanged);
    connect(ui->tbHAlignLeft, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onHAlignChanged);
    connect(ui->tbHalignMiddle, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onHAlignChanged);
    connect(ui->tbHalignRight, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onHAlignChanged);
    connect(ui->tbHAlignNone, &QToolButton::toggled, this, &LC_AlignSingleOptionsWidget::onHAlignChanged);
    connect(ui->cbAlignTo, &QComboBox::currentIndexChanged, this, &LC_AlignSingleOptionsWidget::onAlignToIndexChanged);
}

LC_AlignSingleOptionsWidget::~LC_AlignSingleOptionsWidget() {
    delete ui;
}


void LC_AlignSingleOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyAlignSingle*>(a);

    const int valign = m_action->getVAlign();
    const int halign = m_action->getHAlign();
    const int alignType = m_action->getAlignType();

    LC_GuardedSignalsBlocker({
        ui->cbAlignTo,
        ui->tbHAlignLeft,
        ui->tbHalignMiddle,
        ui->tbHalignRight,
        ui->tbHAlignNone,
        ui->tbVAlignTop,
        ui->tbVAlignBottom,
        ui->tbVAlignMiddle,
        ui->tbValignNone
    });

    switch (halign) {
        case LC_Align::Align::LEFT_TOP: {
            ui->tbHAlignLeft->setChecked(true);
            break;
        }
        case LC_Align::Align::MIDDLE: {
            ui->tbHalignMiddle->setChecked(true);
            break;
        }
        case LC_Align::Align::RIGHT_BOTTOM: {
            ui->tbHalignRight->setChecked(true);
            break;
        }
        default:
            ui->tbHAlignNone->setChecked(true);
    }

    switch (valign) {
        case LC_Align::Align::LEFT_TOP: {
            ui->tbVAlignTop->setChecked(true);
            break;
        }
        case LC_Align::Align::MIDDLE: {
            ui->tbVAlignMiddle->setChecked(true);
            break;
        }
        case LC_Align::Align::RIGHT_BOTTOM: {
            ui->tbVAlignBottom->setChecked(true);
            break;
        }
        default:
            ui->tbValignNone->setChecked(true);
    }
    ui->cbAlignTo->setCurrentIndex(alignType);
}


void LC_AlignSingleOptionsWidget::onVAlignChanged([[maybe_unused]] bool val) const {
    const int valign = getVAlignFromUI();
    m_action->setVAlign(valign);
    m_action->updateOptions();
}

void LC_AlignSingleOptionsWidget::onHAlignChanged([[maybe_unused]] bool val) const {
    const int halign = getHAlignFromUI();
    m_action->setHAlign(halign);
    m_action->updateOptions();
}

void LC_AlignSingleOptionsWidget::onAlignToIndexChanged([[maybe_unused]] int idx) const {
    m_action->setAlignType(ui->cbAlignTo->currentIndex());
    m_action->updateOptions();
}

void LC_AlignSingleOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

int LC_AlignSingleOptionsWidget::getHAlignFromUI() const {
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

int LC_AlignSingleOptionsWidget::getVAlignFromUI() const {
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
