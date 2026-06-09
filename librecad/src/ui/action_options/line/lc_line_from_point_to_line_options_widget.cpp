/****************************************************************************
**
* Options widget for "LineFromPointToLine" action.

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

#include "lc_line_from_point_to_line_options_widget.h"

#include "lc_action_draw_line_from_point_to_line.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_line_from_point_to_line_options_widget.h"

LC_LineFromPointToLineOptionsWidget::LC_LineFromPointToLineOptionsWidget() : ui(new Ui::LC_LineFromPointToLineOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbOrthogonal, &QCheckBox::clicked, this, &LC_LineFromPointToLineOptionsWidget::onOrthogonalClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineFromPointToLineOptionsWidget::onAngleEditingFinished);
    connect(ui->cbSizeMode, &QComboBox::currentIndexChanged, this, &LC_LineFromPointToLineOptionsWidget::onSizeModeIndexChanged);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineFromPointToLineOptionsWidget::onLengthEditingFinished);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_LineFromPointToLineOptionsWidget::onEndOffsetEditingFinished);
    connect(ui->cbSnap, &QComboBox::currentIndexChanged, this, &LC_LineFromPointToLineOptionsWidget::onSnapModeIndexChanged);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("length", ui->tbPickLength, ui->leLength);
    pickDistanceSetup("offset", ui->tbPickOffset, ui->leOffset);
}

LC_LineFromPointToLineOptionsWidget::~LC_LineFromPointToLineOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_LineFromPointToLineOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineFromPointToLineOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineFromPointToLine*>(a);

    const bool orthogonal = m_action->getOrthogonal();
    const int sizeMode = m_action->getSizeMode();
    const int snap = m_action->getLineSnapMode();
    const QString angle = fromDouble(m_action->getAngleDegrees());
    const QString length = fromDouble(m_action->getLength());
    const QString offset = fromDouble(m_action->getEndOffset());

    LC_GuardedSignalsBlocker({ui->cbOrthogonal, ui->leAngle, ui->cbSizeMode, ui->leLength, ui->cbSnap, ui->leOffset});

    ui->cbOrthogonal->setChecked(orthogonal);

    ui->lblAngle->setEnabled(!orthogonal);
    ui->leAngle->setEnabled(!orthogonal);
    ui->tbPickAngle->setEnabled(!orthogonal);

    ui->leAngle->setText(angle);

    ui->cbSizeMode->setCurrentIndex(sizeMode);
    const bool intersectionMode = sizeMode == 0;
    ui->frmLength->setVisible(!intersectionMode);
    ui->tbPickLength->setVisible(!intersectionMode && m_interactiveInputControlsVisible);
    ui->frmOffset->setVisible(intersectionMode);
    ui->tbPickOffset->setVisible(intersectionMode && m_interactiveInputControlsVisible);
    ui->leLength->setText(length);
    ui->cbSnap->setCurrentIndex(snap);
    ui->leOffset->setText(offset);
}

void LC_LineFromPointToLineOptionsWidget::onSnapModeIndexChanged(const int index) const {
    m_action->setLineSnapMode(index);
    m_action->updateOptions();
}

void LC_LineFromPointToLineOptionsWidget::onSizeModeIndexChanged(const int index) const {
    m_action->setSizeMode(index);
    m_action->updateOptions();
}

void LC_LineFromPointToLineOptionsWidget::onOrthogonalClicked(const bool value) const {
    m_action->setOrthogonal(value);
    m_action->updateOptions();
}

void LC_LineFromPointToLineOptionsWidget::onAngleEditingFinished() {
    const QString& value = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(value, angle, 1.0, false)) {
        // ensure angle in 0..180
        const double angleRad = RS_Math::deg2rad(angle);
        const double correctedAngle = std::remainder(angleRad, M_PI);
        angle = RS_Math::rad2deg(std::abs(correctedAngle));
        m_action->setAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_LineFromPointToLineOptionsWidget::onLengthEditingFinished() {
    const QString& value = ui->leLength->text();
    double len;
    if (toDouble(value, len, 1.0, false)) {
        m_action->setLength(len);
    }
    m_action->updateOptions();
}

void LC_LineFromPointToLineOptionsWidget::onEndOffsetEditingFinished() {
    const QString& value = ui->leOffset->text();
    double len;
    if (toDouble(value, len, 0.0, false)) {
        m_action->setEndOffset(len);
    }
    m_action->updateOptions();
}
