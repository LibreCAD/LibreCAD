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

#include "lc_align_ref_options_widget.h"

#include "lc_action_modify_align_ref.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_align_ref_options_widget.h"

LC_AlignRefOptionsWidget::LC_AlignRefOptionsWidget()
    : ui(new Ui::LC_AlignRefOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbScale, &QCheckBox::toggled, this, &LC_AlignRefOptionsWidget::onScaleClicked);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_AlignRefOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_AlignRefOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::clicked, this, &LC_AlignRefOptionsWidget::cbUseCurrentLayerClicked);
}

LC_AlignRefOptionsWidget::~LC_AlignRefOptionsWidget() {
    delete ui;
}

void LC_AlignRefOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyAlignRef*>(a);

    const bool scale = m_action->isScale();
    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const bool keepOriginals = m_action->isKeepOriginals();

    LC_GuardedSignalsBlocker({ui->cbLayer, ui->cbCurrentAttr,     ui->cbScale});
    ui->cbLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);
    ui->cbScale->setChecked(scale);
}

void LC_AlignRefOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_AlignRefOptionsWidget::onScaleClicked([[maybe_unused]] bool clicked) const {
    m_action->setScale(ui->cbScale->isChecked());
    m_action->updateOptions();
}

void LC_AlignRefOptionsWidget::cbKeepOriginalsClicked(const bool val) const {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_AlignRefOptionsWidget::cbUseCurrentAttributesClicked(const bool val) const {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_AlignRefOptionsWidget::cbUseCurrentLayerClicked(const bool val) const {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}
