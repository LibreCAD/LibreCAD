/****************************************************************************
**
* Options widget for "LinePoints" action.

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
#include "lc_points_line_options_widget.h"

#include "lc_action_draw_points_line.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_points_line_options_widget.h"

LC_LinePointsOptionsWidget::LC_LinePointsOptionsWidget() : LC_ActionOptionsWidget(nullptr), ui(new Ui::LC_LinePointsOptionsWidget) {
    ui->setupUi(this);
    connect(ui->sbPointsCount, &QSpinBox::valueChanged, this, &LC_LinePointsOptionsWidget::onPointsCountValueChanged);
    connect(ui->cbEdgePoints, &QComboBox::currentIndexChanged, this, &LC_LinePointsOptionsWidget::onEdgePointsModeIndexChanged);

    connect(ui->cbFixedDistance, &QCheckBox::clicked, this, &LC_LinePointsOptionsWidget::onFixedDistanceClicked);
    connect(ui->cbWithinLine, &QCheckBox::clicked, this, &LC_LinePointsOptionsWidget::onWithinLineClicked);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_LinePointsOptionsWidget::onDistanceEditingFinished);

    connect(ui->cbAngle, &QCheckBox::clicked, this, &LC_LinePointsOptionsWidget::onAngleClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LinePointsOptionsWidget::onAngleEditingFinished);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("distance", ui->tbPickDistance, ui->leDistance);
}

LC_LinePointsOptionsWidget::~LC_LinePointsOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_LinePointsOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawPointsLine*>(a);
    int edgePointMode = 0;
    bool fixedDistanceMode = false;
    bool withinLine = false;
    QString distance;
    QString angle;
    const bool showAllControls = m_action->rtti() == RS2::ActionDrawPointsLine;

    const int direction = m_action->getDirection();
    bool angleMode = direction == LC_AbstractActionDrawLine::DIRECTION_ANGLE;

    if (!showAllControls) {
        angleMode = false;
    }

    const int pointsCount = m_action->getPointsCount();
    if (showAllControls) {
        edgePointMode = m_action->getEdgePointsMode();
        fixedDistanceMode = m_action->isFixedDistanceMode();
        withinLine = m_action->isWithinLineMode();
        distance = fromDouble(m_action->getPointsDistance());
        angle = fromDouble(m_action->getAngleDegrees());
    }

    LC_GuardedSignalsBlocker({
        ui->sbPointsCount,
        ui->cbAngle,
        ui->leAngle,
        ui->cbFixedDistance,
        ui->leDistance,
        ui->cbEdgePoints,
        ui->cbWithinLine
    });

    ui->cbAngle->setVisible(showAllControls);
    ui->leAngle->setVisible(showAllControls);
    ui->tbPickAngle->setVisible(showAllControls && m_interactiveInputControlsVisible);

    ui->line_2->setVisible(showAllControls);
    ui->cbFixedDistance->setVisible(showAllControls);
    ui->frmFixed->setVisible(showAllControls);
    ui->line->setVisible(showAllControls);
    ui->label_2->setVisible(showAllControls);
    ui->cbEdgePoints->setVisible(showAllControls);

    ui->sbPointsCount->setValue(pointsCount);
    ui->cbEdgePoints->setCurrentIndex(edgePointMode);

    ui->cbFixedDistance->setChecked(fixedDistanceMode);
    ui->frmFixed->setVisible(fixedDistanceMode && showAllControls);
    if (!fixedDistanceMode) {
        ui->sbPointsCount->setEnabled(true);
    }
    else {
        ui->sbPointsCount->setEnabled(!withinLine);
    }
    ui->cbWithinLine->setChecked(withinLine);

    ui->leDistance->setText(distance);

    ui->cbAngle->setChecked(angleMode);
    ui->leAngle->setEnabled(angleMode);

    ui->leAngle->setText(angle);
}

void LC_LinePointsOptionsWidget::onPointsCountValueChanged(const int value) const {
    m_action->setPointsCount(value);
    m_action->updateOptions();
}

void LC_LinePointsOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LinePointsOptionsWidget::onEdgePointsModeIndexChanged(const int index) const {
    m_action->setEdgePointsMode(index);
    m_action->updateOptions();
}

void LC_LinePointsOptionsWidget::onFixedDistanceClicked(const bool value) const {
    m_action->setFixedDistanceMode(value);
    m_action->updateOptions();
}

void LC_LinePointsOptionsWidget::onAngleClicked(const bool value) const {
    if (value) {
        m_action->setSetAngleDirectionState();
    }
    else {
        if (m_action->getDirection() == LC_AbstractActionDrawLine::DIRECTION_ANGLE) {
            m_action->setSetPointDirectionState();
        }
    }
    m_action->updateOptions();
}

void LC_LinePointsOptionsWidget::onWithinLineClicked(const bool value) const {
    m_action->setWithinLineMode(value);
    m_action->updateOptions();
}

void LC_LinePointsOptionsWidget::onDistanceEditingFinished() {
    const auto val = ui->leDistance->text();
    double distance;
    if (toDouble(val, distance, 1.0, true)) {
        m_action->setPointsDistance(distance);
    }
    m_action->updateOptions();
}

void LC_LinePointsOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angle;
    if (toDouble(val, angle, 0.0, false)) {
        m_action->setAngleValueDegrees(angle);
    }
    m_action->updateOptions();
}
