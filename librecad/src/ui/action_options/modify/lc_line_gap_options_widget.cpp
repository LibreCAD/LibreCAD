/****************************************************************************
**
* Options widget for action that creates a gap in selected line

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
#include "lc_line_gap_options_widget.h"

#include "lc_action_modify_line_gap.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_line_gap_options_widget.h"

LC_LineGapOptionsWidget::LC_LineGapOptionsWidget() : ui(new Ui::LC_LineGapOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbFree, &QCheckBox::clicked, this, &LC_LineGapOptionsWidget::onFreeGapClicked);
    connect(ui->leSize, &QLineEdit::editingFinished, this, &LC_LineGapOptionsWidget::onSizeEditingFinished);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_LineGapOptionsWidget::onDistanceEditingFinished);
    connect(ui->cbLineSnap, &QComboBox::currentIndexChanged, this, &LC_LineGapOptionsWidget::onLineSnapModeIndexChanged);
    connect(ui->cbGapSnap, &QComboBox::currentIndexChanged, this, &LC_LineGapOptionsWidget::onGapSnapModeIndexChanged);

    pickDistanceSetup("size", ui->tbPickSize, ui->leSize);
    pickDistanceSetup("snap", ui->tbPickSnap, ui->leDistance);
}

LC_LineGapOptionsWidget::~LC_LineGapOptionsWidget() {
    delete ui;
}

void LC_LineGapOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyLineGap*>(a);

    const QString gapSize = fromDouble(m_action->getGapSize());
    const bool gapFree = m_action->isFreeGapSize();
    const int lineSnap = m_action->getLineSnapMode();
    const QString snapDistance = fromDouble(m_action->getGapSnapDistance());
    const int gapSnap = m_action->getGapSnapMode();

    LC_GuardedSignalsBlocker({ui->leSize, ui->cbFree, ui->cbGapSnap, ui->cbLineSnap, ui->leDistance});

    ui->leSize->setText(gapSize);
    ui->cbFree->setChecked(gapFree);

    ui->leSize->setEnabled(!gapFree);
    ui->tbPickSize->setEnabled(!gapFree);
    ui->cbGapSnap->setEnabled(!gapFree);

    ui->cbLineSnap->setCurrentIndex(lineSnap);
    ui->leDistance->setEnabled(lineSnap > 0);

    ui->leDistance->setText(snapDistance);
    ui->cbGapSnap->setCurrentIndex(gapSnap);
}

void LC_LineGapOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineGapOptionsWidget::onFreeGapClicked(const bool val) {
    m_action->setFreeGapSize(val);
    m_action->updateOptions();
}

void LC_LineGapOptionsWidget::onSizeEditingFinished() {
    const auto val = ui->leSize->text();
    double len;
    if (toDouble(val, len, 1.0, false)) {
        m_action->setGapSize(len);
    }
    m_action->updateOptions();
}

void LC_LineGapOptionsWidget::onDistanceEditingFinished() {
    const auto val = ui->leDistance->text();
    double len;
    if (toDouble(val, len, 0.0, false)) {
        m_action->setGapSnapDistance(len);
    }
    m_action->updateOptions();
}

void LC_LineGapOptionsWidget::onLineSnapModeIndexChanged(const int index) {
    m_action->setLineSnapMode(index);
    m_action->updateOptions();
}

void LC_LineGapOptionsWidget::onGapSnapModeIndexChanged(const int index) {
    m_action->setGapSnapMode(index);
    m_action->updateOptions();
}
