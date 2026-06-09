/****************************************************************************
**
* Options widget for Angle Line from line action.

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

#include "lc_line_angle_rel_options_widget.h"

#include "lc_action_draw_line_angle_rel.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_line_angle_rel_options_widget.h"

LC_LineAngleRelOptionsWidget::LC_LineAngleRelOptionsWidget() : LC_ActionOptionsWidget(nullptr),
                                                               ui(std::make_unique<Ui::LC_LineAngleRelOptionsWidget>()) {
    ui->setupUi(this);

    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptionsWidget::onLengthEditingFinished);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptionsWidget::onOffsetEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptionsWidget::onAngleEditingFinished);
    connect(ui->cbRelativeAngle, &QCheckBox::clicked, this, &LC_LineAngleRelOptionsWidget::onAngleRelatedClicked);
    connect(ui->cbDivide, &QCheckBox::clicked, this, &LC_LineAngleRelOptionsWidget::onDivideClicked);
    connect(ui->cbFree, &QCheckBox::clicked, this, &LC_LineAngleRelOptionsWidget::onFreeLengthClicked);
    connect(ui->cbTickSnapMode, &QComboBox::currentIndexChanged, this, &LC_LineAngleRelOptionsWidget::onTickSnapModeIndexChanged);
    connect(ui->cbLineSnapMode, &QComboBox::currentIndexChanged, this, &LC_LineAngleRelOptionsWidget::onLineSnapModeIndexChanged);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptionsWidget::onDistanceEditingFinished);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("length", ui->tbPickLength, ui->leLength);
    pickDistanceSetup("offset", ui->tbPickOffset, ui->leOffset);
    pickDistanceSetup("snapDistance", ui->tbPickSnapDistance, ui->leDistance);
}

LC_LineAngleRelOptionsWidget::~LC_LineAngleRelOptionsWidget() {
    m_action = nullptr;
}

void LC_LineAngleRelOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineAngleRel*>(a);

    const bool fixedAngle = m_action->isFixedAngleActionMode();

    const QString length = fromDouble(m_action->getTickLength());
    const QString offset = fromDouble(m_action->getTickOffset());
    const QString angle = fromDouble(m_action->getTickAngleDegrees());
    const int lineSnapMode = m_action->getLineSnapMode();
    const int tickSnapMode = m_action->getTickSnapMode();
    const bool angleIsRelative = m_action->isAngleRelative();
    const bool lengthIsFree = m_action->isLengthFree();
    const bool divide = m_action->isDivideLine();
    const QString distance = QString("%1").arg(m_action->getIntersectionOffsetDistance());

    LC_GuardedSignalsBlocker({
        ui->leAngle,
        ui->cbRelativeAngle,
        ui->cbFree,
        ui->leLength,
        ui->leOffset,
        ui->leAngle,
        ui->cbRelativeAngle,
        ui->cbLineSnapMode,
        ui->cbTickSnapMode,
        ui->cbDivide,
        ui->leDistance
    });

    ui->leAngle->setVisible(!fixedAngle);
    ui->lblAngle->setVisible(!fixedAngle);
    ui->tbPickAngle->setVisible(!fixedAngle && m_interactiveInputControlsVisible);
    ui->cbRelativeAngle->setVisible(!fixedAngle);
    ui->lnAngleRight->setVisible(!fixedAngle);

    ui->cbFree->setChecked(lengthIsFree);
    ui->leLength->setEnabled(!lengthIsFree);
    ui->tbPickLength->setEnabled(!lengthIsFree);

    ui->leLength->setText(length);
    ui->leOffset->setText(offset);

    if (!fixedAngle) {
        ui->leAngle->setText(angle);
        ui->cbRelativeAngle->setChecked(angleIsRelative);
    }

    ui->cbLineSnapMode->setCurrentIndex(lineSnapMode);
    const bool notFreeSnap = lineSnapMode != 0;
    ui->lblDistance->setVisible(notFreeSnap);
    ui->leDistance->setVisible(notFreeSnap);
    ui->tbPickSnapDistance->setVisible(notFreeSnap && m_interactiveInputControlsVisible);

    ui->cbTickSnapMode->setCurrentIndex(tickSnapMode);
    ui->cbDivide->setChecked(divide);
    ui->leDistance->setText(distance);
}

void LC_LineAngleRelOptionsWidget::onLengthEditingFinished() {
    if (m_action != nullptr) {
        const QString& expr = ui->leLength->text();
        double value = 0.;
        if (toDouble(expr, value, 1.0, false)) {
            m_action->setTickLength(value);
            m_action->updateOptions();
        }
    }
}

void LC_LineAngleRelOptionsWidget::onDistanceEditingFinished() {
    if (m_action != nullptr) {
        const QString& expr = ui->leDistance->text();
        double value = 0.;
        if (toDouble(expr, value, 0.0, false)) {
            m_action->setIntersectionOffsetDistance(value);
            m_action->updateOptions();
        }
    }
}

void LC_LineAngleRelOptionsWidget::onOffsetEditingFinished() {
    if (m_action != nullptr) {
        const QString& expr = ui->leOffset->text();
        double value = 0.;
        if (toDouble(expr, value, 0.0, false)) {
            m_action->setTickOffset(value);
            m_action->updateOptions();
        }
    }
}

void LC_LineAngleRelOptionsWidget::onAngleEditingFinished() {
    if (m_action != nullptr) {
        const QString& expr = ui->leAngle->text();
        double angle = 0.;
        if (toDoubleAngleDegrees(expr, angle, 0.0, false)) {
            m_action->setTickAngleDegrees(angle);
            m_action->updateOptions();
        }
    }
}

void LC_LineAngleRelOptionsWidget::onLineSnapModeIndexChanged(const int index) {
    if (m_action != nullptr) {
        m_action->setLineSnapMode(index);
        m_action->updateOptions();
    }
}

void LC_LineAngleRelOptionsWidget::onTickSnapModeIndexChanged(const int index) {
    if (m_action != nullptr) {
        m_action->setTickSnapMode(index);
        m_action->updateOptions();
    }
}

void LC_LineAngleRelOptionsWidget::onFreeLengthClicked(const bool clicked) {
    if (m_action != nullptr) {
        m_action->setLengthIsFree(clicked);
        m_action->updateOptions();
    }
}

void LC_LineAngleRelOptionsWidget::onAngleRelatedClicked(const bool clicked) {
    if (m_action != nullptr) {
        m_action->setAngleIsRelative(clicked);
        m_action->updateOptions();
    }
}

void LC_LineAngleRelOptionsWidget::onDivideClicked(const bool clicked) {
    if (m_action != nullptr) {
        m_action->setDivideLine(clicked);
        m_action->updateOptions();
    }
}

void LC_LineAngleRelOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
