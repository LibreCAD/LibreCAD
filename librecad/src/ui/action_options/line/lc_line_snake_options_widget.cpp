/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_line_snake_options_widget.h"

#include "lc_action_draw_line_snake.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_line_snake_options_widget.h"

/*
 *  Constructs a QG_LineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineSnakeOptionsWidget::LC_LineSnakeOptionsWidget(): ui(new Ui::LC_LineSnakeOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->rbX, &QRadioButton::clicked, this, &LC_LineSnakeOptionsWidget::onXClicked);
    connect(ui->rbY, &QRadioButton::clicked, this, &LC_LineSnakeOptionsWidget::onYClicked);
    connect(ui->rbPoint, &QRadioButton::clicked, this, &LC_LineSnakeOptionsWidget::onPointClicked);
    connect(ui->rbAngle, &QRadioButton::toggled, this, &LC_LineSnakeOptionsWidget::onAngleClicked);
    connect(ui->cbRelAngle, &QCheckBox::clicked, this, &LC_LineSnakeOptionsWidget::onAngleRelativeClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineSnakeOptionsWidget::onSetAngle);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}


/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineSnakeOptionsWidget::~LC_LineSnakeOptionsWidget() {
    m_action = nullptr;
    delete ui;
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineSnakeOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineSnakeOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineSnake*>(a);

    ui->bClose->setEnabled(m_action->mayClose());
    ui->bUndo->setEnabled(m_action->mayUndo());
    ui->bRedo->setEnabled(m_action->mayRedo());
    ui->bPolyline->setEnabled(m_action->mayClose());

    const int direction = m_action->getDirection();

    LC_GuardedSignalsBlocker({
        ui->rbPoint,
        ui->rbX,
        ui->rbY,
        ui->rbAngle,
        ui->leAngle,
        ui->cbRelAngle,
    });

    ui->rbPoint->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_POINT);
    ui->rbX->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_X);
    ui->rbY->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_Y);
    const bool angleDirection = direction == LC_AbstractActionDrawLine::DIRECTION_ANGLE;

    setupAngleRelatedUI(angleDirection);

    const QString angle = fromDouble(m_action->getAngleDegrees());
    const bool angleRelative = m_action->isAngleRelative();

    ui->leAngle->setText(angle);
    ui->cbRelAngle->setChecked(angleRelative);
}

void LC_LineSnakeOptionsWidget::onXClicked(const bool value) const {
    if (value) {
        m_action->setSetXDirectionState();
        m_action->updateOptions();
    }
}

void LC_LineSnakeOptionsWidget::onYClicked(const bool value) const {
    if (value) {
        m_action->setSetYDirectionState();
        m_action->updateOptions();
    }
}

void LC_LineSnakeOptionsWidget::onPointClicked(const bool value) const {
    if (value) {
        m_action->setSetPointDirectionState();
        m_action->updateOptions();
    }
}

void LC_LineSnakeOptionsWidget::onAngleClicked(const bool value) const {
    if (value) {
        m_action->setSetAngleDirectionState();
        m_action->updateOptions();
    }
}

void LC_LineSnakeOptionsWidget::closeLine() const {
    m_action->close();
}

void LC_LineSnakeOptionsWidget::undo() const {
    m_action->undo();
}

void LC_LineSnakeOptionsWidget::redo() const {
    m_action->redo();
}

void LC_LineSnakeOptionsWidget::polyline() const {
    m_action->polyline();
}

void LC_LineSnakeOptionsWidget::onAngleRelativeClicked(const bool value) const {
    m_action->setAngleIsRelative(value);
    m_action->updateOptions();
}

void LC_LineSnakeOptionsWidget::onSetAngle() {
    const QString& val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
         m_action->setAngleValueDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_LineSnakeOptionsWidget::start() const {
    m_action->setNewStartPointState();
}

void LC_LineSnakeOptionsWidget::setupAngleRelatedUI(const bool value) const {
    ui->rbAngle->setChecked(value);
    ui->leAngle->setEnabled(value);
    ui->tbPickAngle->setEnabled(value);
    ui->cbRelAngle->setEnabled(value);
}
