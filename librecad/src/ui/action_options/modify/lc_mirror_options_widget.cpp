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

#include "lc_mirror_options_widget.h"

#include "lc_action_modify_mirror.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_mirror_options_widget.h"

LC_MirrorOptionsWidget::LC_MirrorOptionsWidget():ui(new Ui::LC_ModifyMirrorOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbMirrorToLine, &QCheckBox::toggled, this, &LC_MirrorOptionsWidget::onMirrorToLineClicked);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_MirrorOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_MirrorOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::clicked, this, &LC_MirrorOptionsWidget::cbUseCurrentLayerClicked);
}

LC_MirrorOptionsWidget::~LC_MirrorOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_MirrorOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyMirror*>(a);

    const bool useLine = m_action->isMirrorToExistingLine();
    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const bool keepOriginals = m_action->isKeepOriginals();

    LC_GuardedSignalsBlocker({ui->cbLayer, ui->cbCurrentAttr, ui->cbKeepOriginals, ui->cbMirrorToLine});

    ui->cbLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);
    ui->cbMirrorToLine->setChecked(useLine);
}

void LC_MirrorOptionsWidget::onMirrorToLineClicked(const bool clicked) {
    m_action->setMirrorToExistingLine(clicked);
    m_action->updateOptions();
}

void LC_MirrorOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_MirrorOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_MirrorOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_MirrorOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}
