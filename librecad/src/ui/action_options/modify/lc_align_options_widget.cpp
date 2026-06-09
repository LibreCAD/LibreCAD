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

#include "lc_align_options_widget.h"

#include "lc_action_modify_align.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_align_options_widget.h"

LC_AlignOptionsWidget::LC_AlignOptionsWidget()
    : ui(new Ui::LC_ModifyAlignOptions) {
    ui->setupUi(this);

    connect(ui->cbAsGroup, &QCheckBox::toggled, this, &LC_AlignOptionsWidget::onAsGroupChanged);
    connect(ui->tbVAlignTop, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onVAlignChanged);
    connect(ui->tbVAlignMiddle, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onVAlignChanged);
    connect(ui->tbVAlignBottom, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onVAlignChanged);
    connect(ui->tbValignNone, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onVAlignChanged);
    connect(ui->tbHAlignLeft, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onHAlignChanged);
    connect(ui->tbHalignMiddle, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onHAlignChanged);
    connect(ui->tbHalignRight, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onHAlignChanged);
    connect(ui->tbHAlignNone, &QToolButton::toggled, this, &LC_AlignOptionsWidget::onHAlignChanged);
    connect(ui->cbAlignTo, &QComboBox::currentIndexChanged, this, &LC_AlignOptionsWidget::onAlignToIndexChanged);
}

LC_AlignOptionsWidget::~LC_AlignOptionsWidget() {
    delete ui;
}

void LC_AlignOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyAlign*>(a);

    const int valign = m_action->getVAlign();
    const int halign = m_action->getHAlign();
    const int alignType = m_action->getAlignType();
    const bool asGroup = m_action->isAsGroup();

    LC_GuardedSignalsBlocker({
        ui->cbAsGroup,
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

    ui->cbAsGroup->setChecked(asGroup);

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

void LC_AlignOptionsWidget::onAsGroupChanged([[maybe_unused]] bool val) const {
    m_action->setAsGroup(ui->cbAsGroup->isChecked());
    m_action->updateOptions();
}

void LC_AlignOptionsWidget::onVAlignChanged([[maybe_unused]] bool val) const {
    const int valign = getVAlignFromUI();
    m_action->setVAlign(valign);
    m_action->updateOptions();
}

void LC_AlignOptionsWidget::onHAlignChanged([[maybe_unused]] bool val) const {
    const int halign = getHAlignFromUI();
    m_action->setHAlign(halign);
    m_action->updateOptions();
}

void LC_AlignOptionsWidget::onAlignToIndexChanged([[maybe_unused]] int idx) const {
    m_action->setAlignType(ui->cbAlignTo->currentIndex());
    m_action->updateOptions();
}

void LC_AlignOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

int LC_AlignOptionsWidget::getHAlignFromUI() const {
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

int LC_AlignOptionsWidget::getVAlignFromUI() const {
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
