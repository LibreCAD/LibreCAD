/****************************************************************************
**
* Options widget for "SliceDivide" action.

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
#include "lc_slice_divide_options_widget.h"

#include <math.h>

#include "lc_action_draw_slice_divide.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_slice_divide_options_widget.h"

LC_SliceDivideOptionsWidget::LC_SliceDivideOptionsWidget() : LC_ActionOptionsWidget(nullptr), ui(new Ui::LC_SliceDivideOptionsWidget) {
    ui->setupUi(this);

    connect(ui->sbCount, &QSpinBox::valueChanged, this, &LC_SliceDivideOptionsWidget::onCountChanged);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_SliceDivideOptionsWidget::onDistanceEditingFinished);
    connect(ui->leTickLengh, &QLineEdit::editingFinished, this, &LC_SliceDivideOptionsWidget::onTickLengthEditingFinished);
    connect(ui->leTickOffset, &QLineEdit::editingFinished, this, &LC_SliceDivideOptionsWidget::onTickOffsetEditingFinished);
    connect(ui->leTickAngle, &QLineEdit::editingFinished, this, &LC_SliceDivideOptionsWidget::onTickAngleEditingFinished);
    connect(ui->leCircleStartAngle, &QLineEdit::editingFinished, this, &LC_SliceDivideOptionsWidget::onCircleStartAngleEditingFinished);
    connect(ui->cbEdgeTick, &QComboBox::currentIndexChanged, this, &LC_SliceDivideOptionsWidget::onDrawTickOnEdgesIndexChanged);
    connect(ui->cbTickSnap, &QComboBox::currentIndexChanged, this, &LC_SliceDivideOptionsWidget::onTickSnapIndexChanged);
    connect(ui->cbRelAngle, &QCheckBox::clicked, this, &LC_SliceDivideOptionsWidget::onRelAngleClicked);
    connect(ui->cbDivide, &QCheckBox::clicked, this, &LC_SliceDivideOptionsWidget::onDivideClicked);
    connect(ui->cbMode, &QCheckBox::clicked, this, &LC_SliceDivideOptionsWidget::onModeClicked);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leTickAngle);
    pickAngleSetup("angleCircle", ui->tbPickCircleAngle, ui->leCircleStartAngle);
    pickDistanceSetup("distance", ui->tbPickLength, ui->leDistance);
    pickDistanceSetup("length", ui->tbPickLength, ui->leTickLengh);
    pickDistanceSetup("offset", ui->tbPickOffset, ui->leTickOffset);
}

LC_SliceDivideOptionsWidget::~LC_SliceDivideOptionsWidget() {
    m_action = nullptr;
    delete ui;
}

// just provide indication to the user that some options are not applicable for selected entity
void LC_SliceDivideOptionsWidget::updateUI(const int mode, [[maybe_unused]] const QVariant* value) {
    switch (mode) {
        case LC_ActionDrawSliceDivide::SELECTION_NONE:
            ui->frmCircle->setEnabled(true);
            ui->frmEdge->setEnabled(true);
            break;
        case LC_ActionDrawSliceDivide::SELECTION_ARC: // arc
            ui->frmCircle->setEnabled(false);
            ui->frmEdge->setEnabled(true);
            break;
        case LC_ActionDrawSliceDivide::SELECTION_CIRCLE: // circle
            ui->frmCircle->setEnabled(true);
            ui->frmEdge->setEnabled(false);
            break;
        default:
            break;
    }
}

void LC_SliceDivideOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawSliceDivide*>(a);

    m_forCircle = a->rtti() == RS2::ActionDrawSliceDivideCircle;

    const int count = m_action->getTickCount();
    const QString tickLen = fromDouble(m_action->getTickLength());
    const QString tickOffset = fromDouble(m_action->getTickOffset());
    const QString tickAngle = fromDouble(m_action->getTickAngleDegrees());
    const QString circleStartAngle = fromDouble(m_action->getCircleStartAngleDegrees());
    const QString distance = fromDouble(m_action->getDistance());
    const int tickSnapMode = m_action->getTickSnapMode();
    const int drawEdgesMode = m_action->getDrawTickOnEdgeMode();
    const bool tickAngleRelative = m_action->isTickAngleRelative();
    const bool divide = m_action->isDivideEntity();
    const bool fixedDistance = m_action->isFixedDistance();

    LC_GuardedSignalsBlocker({
        ui->sbCount,
        ui->leTickLengh,
        ui->leTickOffset,
        ui->leTickAngle,
        ui->leCircleStartAngle,
        ui->cbTickSnap,
        ui->cbEdgeTick,
        ui->cbRelAngle,
        ui->cbDivide,
        ui->cbMode,
        ui->leDistance
    });
    ui->sbCount->setValue(count);
    ui->leTickLengh->setText(tickLen);
    ui->leTickOffset->setText(tickOffset);
    ui->leTickAngle->setText(tickAngle);
    ui->leCircleStartAngle->setText(circleStartAngle);
    ui->cbTickSnap->setCurrentIndex(tickSnapMode);
    ui->cbEdgeTick->setCurrentIndex(drawEdgesMode);
    ui->cbRelAngle->setChecked(tickAngleRelative);
    ui->cbDivide->setChecked(divide);
    ui->cbMode->setChecked(fixedDistance);
    ui->leDistance->setText(distance);

    ui->frmCount->setVisible(!fixedDistance);
    ui->frmDistance->setVisible(fixedDistance);
    ui->frmCircle->setVisible(m_forCircle);
    ui->cbMode->setVisible(!m_forCircle);
    if (m_forCircle) {
        ui->frmDistance->setVisible(false);
        ui->frmCount->setVisible(true);
    }
}

void LC_SliceDivideOptionsWidget::onCountChanged(const int value) {
     m_action->setTickCount(value);
     m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onDistanceEditingFinished() {
    const QString& expr = ui->leDistance->text();
    double value = NAN;
    if (toDouble(expr, value, 1.0, true)) {
        m_action->setDistance(value);
    }
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onTickLengthEditingFinished() {
    const QString& expr = ui->leTickLengh->text();
    double value = NAN;
    if (toDouble(expr, value, 0.0, true)) {
        m_action->setTickLength(value);
    }
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onTickAngleEditingFinished() {
    const QString& expr = ui->leTickAngle->text();
    double angle = NAN;
    if (toDoubleAngleDegrees(expr, angle, 0.0, false)) {
        m_action->setTickAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onTickOffsetEditingFinished() {
    const QString& expr = ui->leTickOffset->text();
    double value = NAN;
    if (toDouble(expr, value, 0.0, false)) {
        m_action->setTickOffset(value);
    }
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onCircleStartAngleEditingFinished() {
    const QString& expr = ui->leCircleStartAngle->text();
    double angle = NAN;
    if (toDoubleAngleDegrees(expr, angle, 0.0, false)) {
        m_action->setCircleStartTickAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onDrawTickOnEdgesIndexChanged(const int index) {
    m_action->setDrawTickOnEdgeMode(index);
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onRelAngleClicked(const bool checked) {
    m_action->setTickAngleRelative(checked);
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onDivideClicked(const bool checked) {
    m_action->setDivideEntity(checked);
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onModeClicked(const bool checked) {
    m_action->setFixedDistance(checked);
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::onTickSnapIndexChanged(const int index) {
     m_action->setTickSnapMode(index);
    m_action->updateOptions();
}

void LC_SliceDivideOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
